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

#include "task.hpp"
#include "job_gen.hpp"

namespace colossal
{

job_generator::job_generator(
	double jar,  double mmpj, double mrpj, double smpj, double srpj,
	double mmtd, double mrtd, double smtd, double srtd)
	: _jar(jar), _jm_mu(mmpj), _jr_mu(mrpj), _jm_sigma(smpj), _jr_sigma(srpj),
	  _tm_mu(mmtd), _tr_mu(mrtd), _tm_sigma(smtd), _tr_sigma(srtd), _now(0)
{
	seed(0);
}

job job_generator::operator()()
{
        job j;
        int nmap = rnmap();
	int nreduce = rnreduce();
        j.id = drand();
	j.fs_ctx_map.uid = j.id;
	j.fs_ctx_reduce.uid = j.id;
	j.fs_ctx_map.weight = 1.0;
	j.fs_ctx_map.minshare = 0;
	j.fs_ctx_reduce.weight = 1.0;
	j.fs_ctx_reduce.minshare = 0;
        _now += rexpo();
	j.ctime = _now;
        for (int i = 0; i < nmap; ++i) {
                task t;
                t.id    = drand();
                t.ctime = j.ctime;
                t.ptime = rmapdur();
                t.stime = -1;
                t.ftime = -1;
                t.type  = task::TASK_TYPE_MAP;
                j.tasks[task::TASK_TYPE_MAP].push_back(t);
        }
        for (int i = 0; i < nreduce; ++i) {
                task t;
                t.id    = drand();
                t.ctime = j.ctime;
                t.ptime = rreducedur();
                t.stime = -1;
                t.ftime = -1;
                t.type  = task::TASK_TYPE_REDUCE;
                j.tasks[task::TASK_TYPE_REDUCE].push_back(t);
        }
        return j;
}

}
