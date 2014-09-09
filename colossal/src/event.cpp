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

#include "log.hpp"
#include "common.hpp"
#include "event.hpp"
#include "engine.hpp"

namespace colossal
{

ev_create_map::ev_create_map(selector *sel)
	: _sel(sel)
{
	// we are guaranteed that sel has map tasks
        _time = sel->map_min_ctime();
}

bool ev_create_map::operator()(engine *eng)
{
	if (_time > eng->time_now)  // possibly woke from sleep
		eng->time_now = _time;

	// creation event is asynchronous, thus time is job tracker time
        DEBUG(eng->time_now, "ev_create_map executed");

	// update demands
	selector::changes_type changes;
	_sel->see_maps(eng->time_now, &changes);

	// update fair shares due to the increased demand
	eng->update_map_fairshares();

	// we may need to add preemption check points accordingly
	for (selector::changes_type::iterator it = changes.begin();
	     it != changes.end(); ++it)
		((td_ref *)(it.key()))->getpool()->map_transit_n2s(eng);

	// acquire resources
	if (!eng->sem_map->wait(this)) {
		DEBUG(eng->time_now, "map creation suspended due to lack of slot");
		return false;
	} else {
		DEBUG(eng->time_now, "map creation acquired a slot");
	}

	// run the map
	td_ref *t = _sel->pop_map();
	if (t == NULL) {
		FATAL(eng->time_now, "popped out a NULL task");
		return true;
	}

	// popping out may change the pool state
	t->getpool()->map_transit_s2n();

	// make the task clean before launching
	t->clear_flag();
	eng->run_map(t);

	// add repeated event
	if (_sel->has_map())
		eng->add_event(new ev_create_map(_sel));
	else {
		DEBUG(eng->time_now, "no more map creation");
	}

	return true;
}

ev_create_reduce::ev_create_reduce(selector *sel)
	: _sel(sel)
{
	// we are guaranteed that sel has reduce tasks
        _time = sel->reduce_min_ctime();
}

bool ev_create_reduce::operator()(engine *eng)
{
	if (_time > eng->time_now)  // possibly woke from sleep
		eng->time_now = _time;

	// creation event is asynchronous, thus time is job tracker time
        DEBUG(eng->time_now, "ev_create_reduce executed");

	// update demands
	selector::changes_type changes;
	_sel->see_reduces(eng->time_now, &changes);

	// update fair shares due to the increased demand
	eng->update_reduce_fairshares();

	// we may need to add preemption check points accordingly
	for (selector::changes_type::iterator it = changes.begin();
	     it != changes.end(); ++it)
		((td_ref *)(it.key()))->getpool()->reduce_transit_n2s(eng);

	// acquire resources
	if (!eng->sem_reduce->wait(this)) {
		DEBUG(eng->time_now, "reduce creation suspended due to lack of slot");
		return false;
	} else {
		DEBUG(eng->time_now, "reduce creation acquired a slot");
	}

	// run the reduce
	td_ref *t = _sel->pop_reduce();
	if (t == NULL) {
		FATAL(eng->time_now, "popped out a NULL task");
		return true;
	}

	// popping out may change the pool state
	t->getpool()->reduce_transit_s2n();

	// make the task clean before launching
	t->clear_flag();
	eng->run_reduce(t);

	// add repeated event
	if (_sel->has_reduce())
		eng->add_event(new ev_create_reduce(_sel));
	else {
		DEBUG(eng->time_now, "no more reduce creation");
	}

	return true;
}

ev_finish_map::ev_finish_map(td_ref *t)
	: _ref(t)
{
	_time = t->gettask()->stime + t->gettask()->ptime;
}

bool ev_finish_map::operator()(engine *eng)
{
	// Only effective if the task has not been preempted
	if (double_equal(_ref->gettask()->stime + _ref->gettask()->ptime, _time) &&
	    !_ref->test_flag(task::TASK_FLAG_PREEMPTED)) {
		eng->time_now = _time;
		eng->finish_map(_ref);
	}
	DEBUG(eng->time_now, "ev_finish_map executed");

	return true;
}

ev_finish_reduce::ev_finish_reduce(td_ref *t)
	: _ref(t)
{
	_time = t->gettask()->stime + t->gettask()->ptime;
}

bool ev_finish_reduce::operator()(engine *eng)
{
	// Only effective if the task has not been preempted and not already finished
	if (double_equal(_ref->gettask()->stime + _ref->gettask()->ptime, _time) &&
	    !_ref->test_flag(task::TASK_FLAG_PREEMPTED)) {
		eng->time_now = _time;
		eng->finish_reduce(_ref);
	}
	DEBUG(eng->time_now, "ev_finish_reduce executed");

	return true;
}

ev_preempt_map::ev_preempt_map(pool *p, double deadline)
	: _pool(p)
{
	_time = deadline;
}

bool ev_preempt_map::operator()(engine *eng)
{
	eng->time_now = _time;

	DEBUG(eng->time_now, "ev_preempt_map executed");

	int ms = _pool->starved_for_map_minshare(_time);
	int hf = _pool->starved_for_map_halffairshare(_time);

	if (ms > hf) {
		NOTICE(eng->time_now, "need to preempt %d maps due to min share", ms);
		eng->preempt_maps(ms);
	} else if (hf > 0) {
		NOTICE(eng->time_now, "need to preempt %d maps due to half fair share", hf);
		eng->preempt_maps(hf);
	}

	return true;
}

ev_preempt_reduce::ev_preempt_reduce(pool *p, double deadline)
	: _pool(p)
{
	_time = deadline;
}

bool ev_preempt_reduce::operator()(engine *eng)
{
	eng->time_now = _time;

	DEBUG(eng->time_now, "ev_preempt_reduce executed");

	int ms = _pool->starved_for_reduce_minshare(_time);
	int hf = _pool->starved_for_reduce_halffairshare(_time);

	if (ms > hf) {
		NOTICE(eng->time_now, "need to preempt %d reduces due to min share", ms);
		eng->preempt_reduces(ms);
	} else if (hf > 0) {
		NOTICE(eng->time_now, "need to preempt %d reduces due to half fair share", hf);
		eng->preempt_reduces(hf);
	}

	return true;
}

}
