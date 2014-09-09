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

#ifndef _COLOSSAL_JOB_GEN_H
#define _COLOSSAL_JOB_GEN_H

#include <cmath>
#include <stdint.h>
#include <algorithm>
#include <ulib/math_rand_prot.h>
#include <ulib/math_rng_normal.h>
#include "job.hpp"

namespace colossal
{

// Job generator using the following distributions:
//   1. Number of maps and reduces in a job follow lognormal distributions;
//   2. Number of submitted jobs in an interval follows a Possion process;
//   3. Map and reduce durations follow lognormal distributions;
class job_generator
{
public:
	// Use the MLE estimators
        // jar:  job arrival rate
        // mmpj: log mean of maps per job
        // mrpj: log mean of reduces per job
        // smpj: log standard deviation of maps per job
        // srpj: log standard deviation of reduces per job
        // mmtd: log mean of map task duration
        // mrtd: log mean of reduce task duration
        // smtd: log standard deviation of map task duration
        // srtd: log standard deviation of reduce task duration
        job_generator(double jar,  double mmpj, double mrpj, double smpj, double srpj,
		      double mmtd, double mrtd, double smtd, double srtd);

        void set_time(double now)
        {
                _now = now;
        }

        void seed(uint64_t s)
        {
                RAND_NR_INIT(_rnorm.u, _rnorm.v, _rnorm.w, s);
        }

	double get_time() const
	{
		return _now;
	}

	// generate a job
        job operator()();

protected:

        // generate a random integer
        uint64_t drand()
        {
                return RAND_NR_NEXT(_rnorm.u, _rnorm.v, _rnorm.w);
        }

        // generate a random value from uniform [0,1] distribution
        double runif()
        {
                return RAND_NR_DOUBLE(drand());
        }

        // generate a random value from exponential distribution
        double rexpo()
        {
                return log(1 - runif())/-_jar;
        }

        // generate a random value from normal distribution
        double rnorm()
        {
                return normal_rng_next(&_rnorm);
        }

        double rmapdur()
        {
                return exp(_tm_mu + rnorm() * _tm_sigma);
        }

        double rreducedur()
        {
                return exp(_tr_mu + rnorm() * _tr_sigma);
        }

        int rnmap()
        {
                return std::max((int)exp(_jm_mu + rnorm() * _jm_sigma), 1);
        }

        int rnreduce()
        {
                return exp(_jr_mu + rnorm() * _jr_sigma);
        }

	double _jar;
	// lognormal parameters for job size
	double _jm_mu, _jr_mu, _jm_sigma, _jr_sigma;
	// lognormal parameters for task duration
	double _tm_mu, _tr_mu, _tm_sigma, _tr_sigma;

        double _now;

        // RNG context
        normal_rng _rnorm;
};

}

#endif
