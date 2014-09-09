/* Colossal
 * Copyright (c) 2014 Zilong Tan (eric.zltan@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, subject to the conditions listed
 * in the Colossal LICENSE file. These conditions include: you must preserve this
 * copyright notice, and you cannot mention the copyright holders in
 * advertising related to the Software without their permission.  The Software
 * is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This notice is a
 * summary of the Colossal LICENSE file; the license in that file is legally
 * binding.
 */

#include <cstddef>
#include <cstdio>
#include <algorithm>
#include <ulib/util_log.h>
#include "log.hpp"
#include "common.hpp"
#include "fsched.hpp"
#include "helper.hpp"
#include "metric.hpp"
#include "engine.hpp"

namespace colossal
{

const double engine::LOAD_FACTOR = 0.7;
const int    engine::PROGRESS_WINSIZE = 50000;

engine::engine(int nmaps, int nreduces, double now)
        : time_now(now), _nmap(nmaps), _nreduce(nreduces),
	  _met_win(0), _fp_met(NULL)
{
	select = NULL; // allocate only when jobs are loaded
        sem_map = new vsem_type(nmaps);
        sem_reduce = new vsem_type(nreduces);
        running_maps = new taskset_type(std::max(nmaps, 2) / LOAD_FACTOR);
        running_reduces = new taskset_type(std::max(nreduces, 2) / LOAD_FACTOR);
}

engine::~engine()
{
        delete sem_map;
        delete sem_reduce;
        delete running_maps;
        delete running_reduces;
	delete select;

	if (_fp_met)
		fclose(_fp_met);
}

bool engine::set_metrics(const char * met, int met_win)
{
	if (met == NULL)
		return false;
	_met_win = met_win;
	_fp_met = fopen(met, "w");
	if (_fp_met == NULL) {
		ULIB_WARNING("cannot open metric file %s", met);
		return false;
	}

	return true;
}

pool &engine::add_pool(const std::string &ns, double mto, double fto,
		       double weight, int minmap, int minred, pool::sched_mode sched)
{
        _pools.push_back(pool(ns, mto, fto, weight, minmap, minred, sched));
        return _pools.back();
}

void engine::run_map(td_ref *t)
{
	// set stime
	t->gettask()->stime = time_now;

	// add to running set
	running_maps->insert(t);

	// add finish event
	add_event(new ev_finish_map(t));
}

void engine::run_reduce(td_ref *t)
{
	// set stime
	t->gettask()->stime = time_now;

	// add to running set
	running_reduces->insert(t);

	// add finish event
	add_event(new ev_finish_reduce(t));
}

void engine::finish_map(td_ref *t)
{
	t->gettask()->ftime = time_now;
	running_maps->erase(t);
	--t->getjob()->fs_ctx_map.alloc;
	--t->getjob()->fs_ctx_map.demand;
	--t->getpool()->fs_ctx_map.alloc;
	--t->getpool()->fs_ctx_map.demand;
	update_map_fairshares(); // since demand has changed, update fair shares
	t->getpool()->map_transit_n2s(this);
	// needed for half fair share starvation
	t->getpool()->map_transit_s2n();
	sem_map->post(this);
}

void engine::finish_reduce(td_ref *t)
{
	t->gettask()->ftime = time_now;
	running_reduces->erase(t);
	--t->getjob()->fs_ctx_reduce.alloc;
	--t->getjob()->fs_ctx_reduce.demand;
	--t->getpool()->fs_ctx_reduce.alloc;
	--t->getpool()->fs_ctx_reduce.demand;
	update_reduce_fairshares(); // since demand has changed, update fair shares
	t->getpool()->reduce_transit_n2s(this);
	// needed for half fair share starvation
	t->getpool()->reduce_transit_s2n();
	sem_reduce->post(this);
}

void engine::add_event(event *ev)
{
	_eventheap.push_back(ev);
	heap_push_inclass(&*_eventheap.begin(), _eventheap.size() - 1,
			  0, *_eventheap.rbegin());
}

void engine::preempt_maps(int num)
{
        int n = 0;
        int m = num;
	running_maps->snap();  // take a snapshop of current running tasks
        running_maps->sort();  // sort the running tasks by start time
        for (taskset_type::iterator it = running_maps->begin();
             it != running_maps->end() && m;) {
                if (((td_ref *)it.key())->getpool()->fs_ctx_map.alloc >
		    ((td_ref *)it.key())->getpool()->fs_ctx_map.fairshare) {
                        ++n;
                        --m;
			((td_ref *)it.key())->set_flag(task::TASK_FLAG_PREEMPTED);
			--((td_ref *)it.key())->getjob()->fs_ctx_map.alloc;
			--((td_ref *)it.key())->getjob()->fs_ctx_map.demand;
			--((td_ref *)it.key())->getpool()->fs_ctx_map.alloc;
			--((td_ref *)it.key())->getpool()->fs_ctx_map.demand;
			// must be added back into the scheduler
			select->add_preempted_map(it.key());
                        running_maps->erase((it++).key());
		} else
			++it;
        }

	// update fair shares due to demand changes
	update_map_fairshares();

	for (int i = 0; i < n; ++i) {
		// wake up pending map creations
		sem_map->post(this);
	}

	NOTICE(time_now, "%d of %d maps have been preempted", n, num);
}

void engine::preempt_reduces(int num)
{
        int n = 0;
        int m = num;
	running_reduces->snap();  // take a snapshop of current running tasks
        running_reduces->sort();  // sort the running tasks by start time
        for (taskset_type::iterator it = running_reduces->begin();
             it != running_reduces->end() && m;) {
                if (((td_ref *)it.key())->getpool()->fs_ctx_reduce.alloc >
		    ((td_ref *)it.key())->getpool()->fs_ctx_reduce.fairshare) {
                        ++n;
                        --m;
			((td_ref *)it.key())->set_flag(task::TASK_FLAG_PREEMPTED);
			--((td_ref *)it.key())->getjob()->fs_ctx_reduce.alloc;
			--((td_ref *)it.key())->getjob()->fs_ctx_reduce.demand;
			--((td_ref *)it.key())->getpool()->fs_ctx_reduce.alloc;
			--((td_ref *)it.key())->getpool()->fs_ctx_reduce.demand;
			// must be added back into the scheduler
			select->add_preempted_reduce(it.key());
                        running_reduces->erase((it++).key());
		} else
			++it;
        }

	// update fair shares due to demand changes
	update_reduce_fairshares();

	for (int i = 0; i < n; ++i) {
		// wake up pending reduce creations
		sem_reduce->post(this);
	}

	NOTICE(time_now, "%d of %d reduces have been preempted", n, num);
}

void engine::submit_tasks()
{
	if (select->has_map())
		add_event(new ev_create_map(select));
	if (select->has_reduce())
		add_event(new ev_create_reduce(select));
}

double engine::map_progress() const
{
	if (select == NULL)
		return 0;
	double total = select->maps_popped() + select->maps_left();
	if (total == 0)
		return 1.0;
	return select->maps_popped() / total;
}

double engine::reduce_progress() const
{
	if (select == NULL)
		return 0;
	double total = select->reduces_popped() + select->reduces_left();
	if (total == 0)
		return 1.0;
	return select->reduces_popped() / total;
}

void engine::process()
{
	// Initially fair shares are zero due to zero demand, and
	// nobody is starved due to zero demands

	// create a task selector on pools
	select = new selector(_pools.begin(), _pools.end());

	// add task creation events
        submit_tasks();

	metric met("", _fp_met);
	size_t nev = 0;
        // process events
	while (_eventheap.size()) {
		event *ev = *_eventheap.begin();
		heap_pop_to_rear_inclass(&*_eventheap.begin(), &*_eventheap.end());
		_eventheap.pop_back();
		if ((*ev)(this))  // delete the event if it is done
			delete ev;
		// sample processing progress
		if (nev % PROGRESS_WINSIZE == 0 || _eventheap.size() == 0)
			show_progress(map_progress(), reduce_progress());
		// sample metrics
		if (_fp_met && (nev % _met_win == 0 || _eventheap.size() == 0)) {
			for (pool_container_type::const_iterator it = _pools.begin();
			     it != _pools.end(); ++it) {
				char key[64];
				snprintf(key, sizeof(key), "%lf", time_now);
				metric root = met[key];
				it->print_metrics(root[it->name]);
			}
		}
		++nev;
	}

	if (nev)
		fprintf(stderr, "\n");
}

void engine::scale_minshares()
{
	map_fs_itr<pool_container_type> map_begin(_pools.begin());
	map_fs_itr<pool_container_type> map_end(_pools.end());
	reduce_fs_itr<pool_container_type> red_begin(_pools.begin());
	reduce_fs_itr<pool_container_type> red_end(_pools.end());
	colossal::scale_minshares(map_begin, map_end, _nmap);
	colossal::scale_minshares(red_begin, red_end, _nreduce);
}

void engine::update_map_fairshares()
{
	map_fs_itr<pool_container_type> begin(_pools.begin());
	map_fs_itr<pool_container_type> end(_pools.end());
	compute_fairshares(begin, end, _nmap);
}

void engine::update_reduce_fairshares()
{
	reduce_fs_itr<pool_container_type> begin(_pools.begin());
	reduce_fs_itr<pool_container_type> end(_pools.end());
	compute_fairshares(begin, end, _nreduce);
}

}
