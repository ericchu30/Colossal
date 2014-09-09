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

#include "job_tracker.hpp"

namespace colossal
{

job_tracker::job_tracker(int nmaps, int nreduces, double now)
{
	_eng = new engine(nmaps, nreduces, now);
}

job_tracker::~job_tracker()
{
	delete _eng;
}

bool job_tracker::set_metrics(const char * met, int met_win)
{
	return _eng->set_metrics(met, met_win);
}

pool & job_tracker::add_pool(const std::string &ns, double mto, double fto,
			     double weight, int minmap, int minred,
			     pool::sched_mode sched)
{
	return _eng->add_pool(ns, mto, fto, weight, minmap, minred, sched);
}

void job_tracker::process()
{
	_eng->process();
}

void job_tracker::scale_minshares()
{
	_eng->scale_minshares();
}

const job_tracker::pool_container_type &job_tracker::getpools() const
{
	return _eng->getpools();
}

job_tracker::pool_container_type &job_tracker::getpools()
{
	return _eng->getpools();
}

}
