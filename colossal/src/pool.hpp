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

#ifndef _COLOSSAL_POOL_H
#define _COLOSSAL_POOL_H

#include <stdint.h>
#include <vector>
#include <string>
#include "job.hpp"
#include "fsched.hpp"
#include "metric.hpp"

namespace colossal
{

class engine;

struct pool
{
	typedef std::vector<job> job_container_type;

	enum sched_mode {
		SCHED_FAIR,
		SCHED_FCFS
	};

        uint64_t id;        // integer unique id, typically a one-to-one mapping to name
        std::string  name;  // human readable string name
	sched_mode  sched;  // scheduling mode for jobs in the pool
        double ms_timeout;  // min share timeout, < 0 to disable
        double hf_timeout;  // half fair share timeout, < 0 to disable
        double map_last_at_ms;  // last time seen below min share
        double map_last_at_hf;  // last time seen below half fair share
        double reduce_last_at_ms;  // last time seen below min share
        double reduce_last_at_hf;  // last time seen below half fair share
        fs_context fs_ctx_map;    // fair scheduling context
        fs_context fs_ctx_reduce; // fair schedulign context
        job_container_type jobs;    // all jobs records in the pool

        // timeout < 0 disables preemption
        pool(const std::string &ns, double mto, double fto,
             double weight, int minmap, int minred, sched_mode sched);

	static uint64_t id_from_str(const char *str);

	job &add_job(const job &j);

	// returns the number of needed slots if starved for minimum share, 0 otherwise
	int starved_for_map_minshare(double now) const;
	int starved_for_reduce_minshare(double now) const;
	// returns the number of needed slots if starved for half fair share, 0 otherwise
	int starved_for_map_halffairshare(double now) const;
	int starved_for_reduce_halffairshare(double now) const;

	// transitions from normal to starved
        void map_transit_n2s(engine *eng);
        void reduce_transit_n2s(engine *eng);

	// transitions from starved to normal
        void map_transit_s2n();
        void reduce_transit_s2n();

	std::string to_str() const;
	void print_metrics(metric met) const;
};

}

#endif
