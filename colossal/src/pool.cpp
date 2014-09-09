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

#include <cstring>
#include <algorithm>
#include <ulib/hash_func.h>
#include "event.hpp"
#include "engine.hpp"
#include "pool.hpp"

namespace colossal
{

pool::pool(const std::string &ns, double mto, double fto,
	   double weight, int minmap, int minred, sched_mode sc)
{
	id = id_from_str(ns.c_str());
	name = ns;
	sched = sc;
	ms_timeout = mto;
	hf_timeout = fto;
	map_last_at_ms = -1;  // < 0 indicates not starved
	map_last_at_hf = -1;  // < 0 indicates not starved
	reduce_last_at_ms = -1;  // < 0 indicates not starved
	reduce_last_at_hf = -1;  // < 0 indicates not starved
	fs_ctx_map.uid = id;
	fs_ctx_reduce.uid = id;
	fs_ctx_map.weight = weight;
	fs_ctx_reduce.weight = weight;
	fs_ctx_map.minshare = minmap;
	fs_ctx_reduce.minshare = minred;
}

job &pool::add_job(const job &j)
{
	// demand is set by task creation events
	jobs.push_back(j);
	return jobs.back();
}

uint64_t pool::id_from_str(const char *str)
{
	return hash_fast64(str, strlen(str), 0xdeedbeeffeedbeefull);
}

// returns the number of needed slots if starved for minimum share, 0 otherwise
int pool::starved_for_map_minshare(double now) const
{
	if (map_last_at_ms < 0 || now - map_last_at_ms < ms_timeout)
		return 0;

	int need = std::min(fs_ctx_map.demand, (int)fs_ctx_map.minshare) - fs_ctx_map.alloc;
	if (need <= 0) {
		ULIB_FATAL("expected allocation below minshare");
		return 0;
	}

	return need;
}

int pool::starved_for_reduce_minshare(double now) const
{
	if (reduce_last_at_ms < 0 || now - reduce_last_at_ms < ms_timeout)
		return 0;

	int need = std::min(fs_ctx_reduce.demand, (int)fs_ctx_reduce.minshare) - fs_ctx_reduce.alloc;
	if (need <= 0) {
		ULIB_FATAL("expected allocation below minshare");
		return 0;
	}

	return need;
}

// returns the number of needed slots if starved for half fair share, 0 otherwise
int pool::starved_for_map_halffairshare(double now) const
{
	if (map_last_at_hf < 0 || now - map_last_at_hf < hf_timeout)
		return 0;

	if (fs_ctx_map.alloc >= (int)(fs_ctx_map.fairshare / 2.0)) {
		ULIB_FATAL("expected allocation below half fair share");
		return 0;
	}

	return fs_ctx_map.fairshare - fs_ctx_map.alloc;
}

int pool::starved_for_reduce_halffairshare(double now) const
{
	if (reduce_last_at_hf < 0 || now - reduce_last_at_hf < hf_timeout)
		return 0;

	if (fs_ctx_reduce.alloc >= (int)(fs_ctx_reduce.fairshare / 2.0)) {
		ULIB_FATAL("expected allocation below half fair share");
		return 0;
	}

	return fs_ctx_reduce.fairshare - fs_ctx_reduce.alloc;
}

void pool::map_transit_n2s(engine *eng)
{
	bool below_ms = fs_ctx_map.alloc < std::min(fs_ctx_map.demand, (int)fs_ctx_map.minshare);
	bool below_hf = fs_ctx_map.alloc < (int)(fs_ctx_map.fairshare / 2.0);

	// add checkpoints if necessary
	if (ms_timeout >= 0 && map_last_at_ms < 0 && below_ms) {
		map_last_at_ms = eng->time_now;
		eng->add_event(new ev_preempt_map(this, eng->time_now + ms_timeout));
	}
	if (hf_timeout >= 0 && map_last_at_hf < 0 && below_hf) {
		map_last_at_hf = eng->time_now;
		eng->add_event(new ev_preempt_map(this, eng->time_now + hf_timeout));
	}
}

void pool::map_transit_s2n()
{
	bool below_ms = fs_ctx_map.alloc < std::min(fs_ctx_map.demand, (int)fs_ctx_map.minshare);
	bool below_hf = fs_ctx_map.alloc < (int)(fs_ctx_map.fairshare / 2.0);

	// update last seen starving times
	if (!below_ms)
		map_last_at_ms = -1;
	if (!below_hf)
		map_last_at_hf = -1;
}

void pool::reduce_transit_n2s(engine *eng)
{
	bool below_ms = fs_ctx_reduce.alloc < std::min(fs_ctx_reduce.demand, (int)fs_ctx_reduce.minshare);
	bool below_hf = fs_ctx_reduce.alloc < (int)(fs_ctx_reduce.fairshare / 2.0);

	// add checkpoints if necessary
	if (ms_timeout >= 0 && reduce_last_at_ms < 0 && below_ms) {
		reduce_last_at_ms = eng->time_now;
		eng->add_event(new ev_preempt_reduce(this, eng->time_now + ms_timeout));
	}
	if (hf_timeout >= 0 && reduce_last_at_hf < 0 && below_hf) {
		reduce_last_at_hf = eng->time_now;
		eng->add_event(new ev_preempt_reduce(this, eng->time_now + hf_timeout));
	}
}

void pool::reduce_transit_s2n()
{
	bool below_ms = fs_ctx_reduce.alloc < std::min(fs_ctx_reduce.demand, (int)fs_ctx_reduce.minshare);
	bool below_hf = fs_ctx_reduce.alloc < (int)(fs_ctx_reduce.fairshare / 2.0);

	// update last seen starving times
	if (!below_ms)
		reduce_last_at_ms = -1;
	if (!below_hf)
		reduce_last_at_hf = -1;
}

std::string pool::to_str() const
{
	char idstr[32];

	snprintf(idstr, sizeof(idstr), "|%016llx", id);
	std::string s = name + idstr;
	for (job_container_type::const_iterator it = jobs.begin();
	     it != jobs.end(); ++it)
		s += "\n\t" + it->to_str("\t\t");

	return s;
}

void pool::print_metrics(metric met) const
{
	metric met_map = met["map"];
	metric met_red = met["reduce"];

	met_map["demand"].set_value(fs_ctx_map.demand);
	met_red["demand"].set_value(fs_ctx_reduce.demand);
	met_map["fairShare"].set_value(fs_ctx_map.fairshare);
	met_red["fairShare"].set_value(fs_ctx_reduce.fairshare);
	met_map["lastTimeAtMinShare"].set_value(map_last_at_ms);
	met_red["lastTimeAtMinShare"].set_value(reduce_last_at_ms);
	met_map["lastTimeAtHalfFairShare"].set_value(map_last_at_hf);
	met_red["lastTimeAtHalfFairShare"].set_value(reduce_last_at_hf);
	met_map["minShare"].set_value(fs_ctx_map.minshare);
	met_red["minShare"].set_value(fs_ctx_reduce.minshare);
	met_map["runningTasks"].set_value(fs_ctx_map.alloc);
	met_red["runningTasks"].set_value(fs_ctx_reduce.alloc);
	met_map["weight"].set_value(fs_ctx_map.weight);
	met_red["weight"].set_value(fs_ctx_reduce.weight);
}

}
