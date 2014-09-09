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

#include <cstdio>
#include <ulib/util_log.h>
#include "fsched.hpp"
#include "selector.hpp"

namespace colossal {

selector::selector(const pool_itr_type &pb, const pool_itr_type &pe)
	: _pb(pb), _pe(pe), _maps_popped(0), _reduces_popped(0)
{
	for (pool_itr_type pit = pb; pit != pe; ++pit) {
		for (pool::job_container_type::iterator jit = pit->jobs.begin();
		     jit != pit->jobs.end(); ++jit) {
			for (job::task_container_type::iterator tit = jit->tasks[task::TASK_TYPE_MAP].begin();
			     tit != jit->tasks[task::TASK_TYPE_MAP].end(); ++tit) {
				task_desc *td = new task_desc(&*tit, &*jit, &*pit);
				td_ref *p = new td_ref(td);
				_map_refs.push_back(p);
			}
			for (job::task_container_type::iterator tit = jit->tasks[task::TASK_TYPE_REDUCE].begin();
			     tit != jit->tasks[task::TASK_TYPE_REDUCE].end(); ++tit) {
				task_desc *td = new task_desc(&*tit, &*jit, &*pit);
				td_ref *p  = new td_ref(td);
				_reduce_refs.push_back(p);
			}
		}
	}
	heap_init_inclass(&*_map_refs.begin(), &*_map_refs.end());
	heap_init_inclass(&*_reduce_refs.begin(), &*_reduce_refs.end());
}

void selector::add_preempted_map(td_ref *ref)
{
	// deep copy to avoid double-free
	td_ref *p = new td_ref(*ref);

	_map_refs.push_back(p);
	heap_push_inclass(&*_map_refs.begin(), _map_refs.size() - 1, 0, p);
}

void selector::add_preempted_reduce(td_ref *ref)
{
	// deep copy to avoid double-free
	td_ref *p = new td_ref(*ref);

	_reduce_refs.push_back(p);
	heap_push_inclass(&*_reduce_refs.begin(), _reduce_refs.size() - 1, 0, p);
}

selector::~selector()
{
	// free remaining refs
	for (p2j_type::iterator pit = _map_tasks.begin();
	     pit != _map_tasks.end(); ++pit) {
		// visit each job in the job hash map
		for (j2t_type::iterator jit = pit.value()->begin();
		     jit != pit.value()->end(); ++jit) {
			// free the task list, task refs will be freed later
			delete jit.value();
		}
		// free job hash map
		delete pit.value();
	}
	for (p2j_type::iterator pit = _reduce_tasks.begin();
	     pit != _reduce_tasks.end(); ++pit) {
		for (j2t_type::iterator jit = pit.value()->begin();
		     jit != pit.value()->end(); ++jit)
			delete jit.value();
		delete pit.value();
	}
	// free task refs
	for (std::vector<td_ref *>::iterator it = _map_refs.begin();
	     it != _map_refs.end(); ++it)
		delete *it;
	for (std::vector<td_ref *>::iterator it = _reduce_refs.begin();
	     it != _reduce_refs.end(); ++it)
		delete *it;
	for (std::vector<td_ref *>::iterator it = _seen_maps.begin();
	     it != _seen_maps.end(); ++it)
		delete *it;
	for (std::vector<td_ref *>::iterator it = _seen_reduces.begin();
	     it != _seen_reduces.end(); ++it)
		delete *it;
}

void selector::dump_seen_task_tree() const
{
	printf("[Begin dumping seen task tree]\n");
	printf("[MAP] %llu pools\n", _map_tasks.size());
	for (p2j_type::const_iterator pit = _map_tasks.begin();
	     pit != _map_tasks.end(); ++pit) {
		printf("    [POOL] %s has seen %llu jobs, A/D=%d/%d\n",
		       pit.key().ptr->getpool()->name.c_str(), pit.value()->size(),
		       pit.key().ptr->getpool()->fs_ctx_map.alloc,
		       pit.key().ptr->getpool()->fs_ctx_map.demand);
		// visit each job in the job hash map
		for (j2t_type::const_iterator jit = pit.value()->begin();
		     jit != pit.value()->end(); ++jit) {
			printf("        [JOB] %016llx has %lu tasks, A/D=%d/%d\n",
			       jit.key().ptr->getjob()->id, jit.value()->size(),
			       jit.key().ptr->getjob()->fs_ctx_map.alloc,
			       jit.key().ptr->getjob()->fs_ctx_map.demand);
		}
	}
	printf("[REDUCE] %llu pools\n", _reduce_tasks.size());
	for (p2j_type::const_iterator pit = _reduce_tasks.begin();
	     pit != _reduce_tasks.end(); ++pit) {
		printf("    [POOL] %s has seen %llu jobs, A/D=%d/%d\n",
		       pit.key().ptr->getpool()->name.c_str(), pit.value()->size(),
		       pit.key().ptr->getpool()->fs_ctx_reduce.alloc,
		       pit.key().ptr->getpool()->fs_ctx_reduce.demand);
		for (j2t_type::const_iterator jit = pit.value()->begin();
		     jit != pit.value()->end(); ++jit) {
			printf("        [JOB] %016llx has %lu tasks, A/D=%d/%d\n",
			       jit.key().ptr->getjob()->id, jit.value()->size(),
			       jit.key().ptr->getjob()->fs_ctx_reduce.alloc,
			       jit.key().ptr->getjob()->fs_ctx_reduce.demand);
		}
	}
	printf("[End dumping seen task tree]\n");
}

double selector::map_min_ctime() const
{
	if (_maps_popped == _seen_maps.size()) {
		if (!_map_refs.size())
			return -1; // no more maps
		return (*_map_refs.begin())->gettask()->ctime;
	}
	// search for buffered tasks with the minimum ctime
	for (std::vector<td_ref *>::const_iterator it = _seen_maps.begin();
	     it != _seen_maps.end(); ++it) {
		if (!(*it)->test_flag(task::TASK_FLAG_POPPED))
			return (*it)->gettask()->ctime;
	}
	ULIB_FATAL("unexpected all popped tasks");
	return -1;
}

double selector::reduce_min_ctime() const
{
	if (_reduces_popped == _seen_reduces.size()) {
		if (!_reduce_refs.size())
			return -1; // no more reduces
		return (*_reduce_refs.begin())->gettask()->ctime;
	}
	// search for buffered tasks with the minimum ctime
	for (std::vector<td_ref *>::const_iterator it = _seen_reduces.begin();
	     it != _seen_reduces.end(); ++it) {
		if (!(*it)->test_flag(task::TASK_FLAG_POPPED))
			return (*it)->gettask()->ctime;
	}
	ULIB_FATAL("unexpected all popped tasks");
	return -1;
}

void selector::see_maps(double now, changes_type *changes)
{
	// move emerged (ctime <= now) tasks to task tree
	while (_map_refs.size() &&
	       (*_map_refs.begin())->gettask()->ctime <= now) {  // just seen top
		td_ref *top = *_map_refs.begin();
		heap_pop_to_rear_inclass(&*_map_refs.begin(), &*_map_refs.end());
		_map_refs.pop_back();
		_seen_maps.push_back(top);
		// find or create the pool-to-job mapping
		p2j_type::iterator pit = _map_tasks.find(top);
		if (pit == _map_tasks.end()) {
			j2t_type *jm = new j2t_type;
			pit = _map_tasks.insert(top, jm);
		}
		// find or create the job-to-task mapping
		j2t_type::iterator jit = pit.value()->find(top);
		if (jit == pit.value()->end()) {
			std::queue<td_ref *> *pl = new std::queue<td_ref *>;
			jit = pit.value()->insert(top, pl);
		}
		jit.value()->push(top);
		if (changes)
			changes->insert(top);
		++top->getpool()->fs_ctx_map.demand;
		++top->getjob()->fs_ctx_map.demand;
	}
}

td_ref * selector::job_select_map_fs(pool_map_fs_itr pchosen)
{
	job_map_fs_itr jbegin(pchosen.itr.value()->begin());
	job_map_fs_itr jend(pchosen.itr.value()->end());
	fs_select<job_map_fs_itr> jfs(jbegin, jend);
	job_map_fs_itr jchosen = jfs();
	if (jchosen == jend) {
		ULIB_FATAL("should have chosen from a non-empty job");
		return NULL;
	}

	td_ref *ret = jchosen.itr.value()->front();
	jchosen.itr.value()->pop();

	// remove inactive job
	job *j = jchosen.itr.key().ptr->getjob();
	if (j->fs_ctx_map.alloc == j->fs_ctx_map.demand) {
		if (jchosen.itr.value()->size())
			ULIB_FATAL("task set is non-empty while removing the job");
		delete jchosen.itr.value();
		// the job set will be freed if its belonging pool is inactive
		pchosen.itr.value()->erase(jchosen.itr);
	}

	return ret;
}

td_ref * selector::job_select_map_fcfs(pool_map_fs_itr pchosen)
{
	job_map_fcfs_itr jbegin(pchosen.itr.value()->begin());
	job_map_fcfs_itr jend(pchosen.itr.value()->end());
	fs_select<job_map_fcfs_itr> jfs(jbegin, jend);
	job_map_fcfs_itr jchosen = jfs();
	if (jchosen == jend) {
		ULIB_FATAL("should have chosen from a non-empty job");
		return NULL;
	}

	td_ref *ret = jchosen.itr.value()->front();
	jchosen.itr.value()->pop();

	// remove inactive job
	job *j = jchosen.itr.key().ptr->getjob();
	if (j->fs_ctx_map.alloc == j->fs_ctx_map.demand) {
		if (jchosen.itr.value()->size())
			ULIB_FATAL("task set is non-empty while removing the job");
		delete jchosen.itr.value();
		// the job set will be freed if its belonging pool is inactive
		pchosen.itr.value()->erase(jchosen.itr);
	}

	return ret;
}

// pop out a map/reduce task
td_ref *selector::pop_map()
{
	if (_maps_popped == _seen_maps.size()) {
		ULIB_DEBUG("haven't seen a new task");
		return NULL;
	}

	pool_map_fs_itr pbegin(_map_tasks.begin());
	pool_map_fs_itr pend(_map_tasks.end());
	fs_select<pool_map_fs_itr> pfs(pbegin, pend);
	pool_map_fs_itr pchosen = pfs();
	if (pchosen == pend) {
		ULIB_FATAL("should have chosen a task");
		return NULL;
	}

	pool *p = pchosen.itr.key().ptr->getpool();
	td_ref *ret;
	if (p->sched == pool::SCHED_FAIR)
		ret = job_select_map_fs(pchosen);
	else if (p->sched == pool::SCHED_FCFS)
		ret = job_select_map_fcfs(pchosen);
	else {
		ULIB_FATAL("unrecognized sched mode:%d for pool %s", p->sched, p->name.c_str());
		return NULL;
	}

	// mark the task as 'popped'
	ret->set_flag(task::TASK_FLAG_POPPED);
	++_maps_popped;

	// remove inactive pool
	if (p->fs_ctx_map.alloc == p->fs_ctx_map.demand) {
		if (pchosen.itr.value()->size())
			ULIB_FATAL("job set is non-empty while removing the pool");
		delete pchosen.itr.value();
		_map_tasks.erase(pchosen.itr);
	}

	return ret;
}

td_ref *selector::pop_map(double now)
{
	see_maps(now);
	return pop_map();
}

void selector::see_reduces(double now, changes_type *changes)
{
	// move emerged (ctime <= now) tasks to task tree
	while (_reduce_refs.size() &&
	       (*_reduce_refs.begin())->gettask()->ctime <= now) {  // just seen top
		td_ref *top = *_reduce_refs.begin();
		heap_pop_to_rear_inclass(&*_reduce_refs.begin(), &*_reduce_refs.end());
		_reduce_refs.pop_back();
		_seen_reduces.push_back(top);
		// find or create the pool-to-job mapping
		p2j_type::iterator pit = _reduce_tasks.find(top);
		if (pit == _reduce_tasks.end()) {
			j2t_type *jm = new j2t_type;
			pit = _reduce_tasks.insert(top, jm);
		}
		// find or create the job-to-task mapping
		j2t_type::iterator jit = pit.value()->find(top);
		if (jit == pit.value()->end()) {
			std::queue<td_ref *> *pl = new std::queue<td_ref *>;
			jit = pit.value()->insert(top, pl);
		}
		jit.value()->push(top);
		if (changes)
			changes->insert(top);
		++top->getpool()->fs_ctx_reduce.demand;
		++top->getjob()->fs_ctx_reduce.demand;
	}
}

td_ref * selector::job_select_reduce_fs(pool_reduce_fs_itr pchosen)
{
	job_reduce_fs_itr jbegin(pchosen.itr.value()->begin());
	job_reduce_fs_itr jend(pchosen.itr.value()->end());
	fs_select<job_reduce_fs_itr> jfs(jbegin, jend);
	job_reduce_fs_itr jchosen = jfs();
	if (jchosen == jend) {
		ULIB_FATAL("should have chosen from a non-empty job");
		return NULL;
	}

	td_ref *ret = jchosen.itr.value()->front();
	jchosen.itr.value()->pop();

	// remove inactive job
	job *j = jchosen.itr.key().ptr->getjob();
	if (j->fs_ctx_reduce.alloc == j->fs_ctx_reduce.demand) {
		if (jchosen.itr.value()->size())
			ULIB_FATAL("task set is non-empty while removing the job");
		delete jchosen.itr.value();
		// the job set will be freed if its belonging pool is inactive
		pchosen.itr.value()->erase(jchosen.itr);
	}

	return ret;
}

td_ref * selector::job_select_reduce_fcfs(pool_reduce_fs_itr pchosen)
{
	job_reduce_fcfs_itr jbegin(pchosen.itr.value()->begin());
	job_reduce_fcfs_itr jend(pchosen.itr.value()->end());
	fs_select<job_reduce_fcfs_itr> jfs(jbegin, jend);
	job_reduce_fcfs_itr jchosen = jfs();
	if (jchosen == jend) {
		ULIB_FATAL("should have chosen from a non-empty job");
		return NULL;
	}

	td_ref *ret = jchosen.itr.value()->front();
	jchosen.itr.value()->pop();

	// remove inactive job
	job *j = jchosen.itr.key().ptr->getjob();
	if (j->fs_ctx_reduce.alloc == j->fs_ctx_reduce.demand) {
		if (jchosen.itr.value()->size())
			ULIB_FATAL("task set is non-empty while removing the job");
		delete jchosen.itr.value();
		// the job set will be freed if its belonging pool is inactive
		pchosen.itr.value()->erase(jchosen.itr);
	}

	return ret;
}

// pop out a reduce/reduce task
td_ref *selector::pop_reduce()
{
	if (_reduces_popped == _seen_reduces.size()) {
		ULIB_DEBUG("haven't seen a new task");
		return NULL;
	}

	pool_reduce_fs_itr pbegin(_reduce_tasks.begin());
	pool_reduce_fs_itr pend(_reduce_tasks.end());
	fs_select<pool_reduce_fs_itr> pfs(pbegin, pend);
	pool_reduce_fs_itr pchosen = pfs();
	if (pchosen == pend) {
		ULIB_FATAL("should have chosen a task");
		return NULL;
	}

	pool *p = pchosen.itr.key().ptr->getpool();
	td_ref *ret;
	if (p->sched == pool::SCHED_FAIR)
		ret = job_select_reduce_fs(pchosen);
	else if (p->sched == pool::SCHED_FCFS)
		ret = job_select_reduce_fcfs(pchosen);
	else {
		ULIB_FATAL("unrecognized sched mode:%d for pool %s", p->sched, p->name.c_str());
		return NULL;
	}

	// mark the task as 'popped'
	ret->set_flag(task::TASK_FLAG_POPPED);
	++_reduces_popped;

	// remove inactive pool
	if (p->fs_ctx_reduce.alloc == p->fs_ctx_reduce.demand) {
		if (pchosen.itr.value()->size())
			ULIB_FATAL("job set is non-empty while removing the pool");
		delete pchosen.itr.value();
		_reduce_tasks.erase(pchosen.itr);
	}

	return ret;
}

td_ref *selector::pop_reduce(double now)
{
	see_reduces(now);
	return pop_reduce();
}

}
