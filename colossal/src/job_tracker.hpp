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

#ifndef _COLOSSAL_JOB_TRACKER_H
#define _COLOSSAL_JOB_TRACKER_H

#include "pool.hpp"
#include "engine.hpp"

namespace colossal
{

class job_tracker {
public:
	typedef engine::pool_container_type pool_container_type;

	job_tracker(int nmaps,        // Number of map slots in the cluster
		    int nreduces,     // Number of reduce slots in the cluster
		    double now = 0);  // Boot time of the job tracker

	virtual ~job_tracker();

	// Set the output metric file and sampling window size
	bool set_metrics(const char * met, int met_win);

	// Add a pool to the engine
	pool &add_pool(const std::string &ns, double mto, double fto,
		       double weight, int minmap, int minred,
		       pool::sched_mode sched);

	// Start processing all jobs
	void process();

	// Scale map and reduce min shares
	// Required if min shares exceed the total number of slots
	void scale_minshares();

	const pool_container_type &getpools() const;
	pool_container_type &getpools();

protected:
	engine *_eng;
};

}

#endif
