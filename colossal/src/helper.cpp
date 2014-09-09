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
#include <stdint.h>
#include <cstring>
#include <vector>
#include <functional>
#include <ulib/heap_prot.h>
#include <ulib/hash_func.h>
#include <ulib/hash_open.h>
#include <ulib/util_log.h>
#include "job.hpp"
#include "pool.hpp"
#include "helper.hpp"

namespace colossal {

void print_pool_settings(const job_tracker::pool_container_type &pools)
{
	for (job_tracker::pool_container_type::const_iterator it = pools.begin();
	     it != pools.end(); ++it) {
		printf("Pool name = %s\tid = %016llx\tnjobs = %zu\tsched = %s\n",
		       it->name.c_str(), it->id, it->jobs.size(),
		       it->sched == pool::SCHED_FAIR? "FAIR": "FCFS");
		printf("\tw = %lf\tm = %lf\td = %d\tmt = %lf\tft = %lf\n",
		       it->fs_ctx_map.weight, it->fs_ctx_map.minshare, it->fs_ctx_map.demand,
		       it->ms_timeout, it->hf_timeout);
		printf("\tr = %lf\ta = %d\n",
		       it->fs_ctx_map.fairshare, it->fs_ctx_map.alloc);
		printf("\tw = %lf\tm = %lf\td = %d\tmt = %lf\tft = %lf\n",
		       it->fs_ctx_reduce.weight, it->fs_ctx_reduce.minshare, it->fs_ctx_reduce.demand,
		       it->ms_timeout, it->hf_timeout);
		printf("\tr = %lf\ta = %d\n",
		       it->fs_ctx_reduce.fairshare, it->fs_ctx_reduce.alloc);
	}
}

void show_progress(double map, double reduce)
{
	int const BARLEN = 28;
	char mbar[BARLEN + 1];
	char rbar[BARLEN + 1];

	int mdone = map * BARLEN;
	int rdone = reduce * BARLEN;

	memset(mbar, '#', mdone);
	memset(rbar, '#', rdone);
	memset(mbar + mdone, ' ', sizeof(mbar) - mdone);
	memset(rbar + rdone, ' ', sizeof(rbar) - rdone);
	mbar[sizeof(mbar) - 1] = '\0';
	rbar[sizeof(rbar) - 1] = '\0';

	fprintf(stderr, "MAP %3d%%[%s] REDUCE %3d%%[%s]\r",
		(int)(map * 100), mbar,	(int)(reduce * 100), rbar);
}

double compute_utilization(
	const job_tracker::pool_container_type &pools,
	task::task_type type, int nslots)
{
	std::vector<ut_pt> points;

	for (job_tracker::pool_container_type::const_iterator pit = pools.begin();
	     pit != pools.end(); ++pit) {
		for (pool::job_container_type::const_iterator jit = pit->jobs.begin();
		     jit != pit->jobs.end(); ++jit) {
			for (job::task_container_type::const_iterator tit = jit->tasks[type].begin();
			     tit != jit->tasks[type].end(); ++tit) {
				points.push_back(tit->stime);
				points.push_back(-tit->ftime);
			}
		}
	}
	if (points.size() == 0) {
		ULIB_DEBUG("no task, use default utilization 0");
		return 0;
	}
	if (points.size() & 1) {
		ULIB_FATAL("expect even number of points, got %zu", points.size());
		return -1;
	}
	heap_init_ut(&*points.begin(), &*points.end());

	double first = points.begin()->time;
	double last  = first < 0? -first: first;
	double util  = 0;
	int    in    = first < 0? -1: 1;
	heap_pop_to_rear_ut(&*points.begin(), &*points.end());
	points.pop_back();
	do {
		double t = points.begin()->time;
		double s = t < 0? -t: t;
		if ((in < 0 || in > nslots) && !double_equal(s, last))
			ULIB_FATAL("used slots(%d) is illegal between %f and %f", in, last, s);
		util += in * (s - last);
		if (t < 0)
			--in;
		else
			++in;
		last = s;
		heap_pop_to_rear_ut(&*points.begin(), &*points.end());
		points.pop_back();
	} while (points.size());

	if (in) {
		ULIB_FATAL("used slots(%d) should be zero in the end", in);
		return -1;
	}

	return util / (last - first) / nslots;
}

double compute_utilization(const pool &p, task::task_type type, int nslots)
{
	std::vector<ut_pt> points;

	for (pool::job_container_type::const_iterator jit = p.jobs.begin();
	     jit != p.jobs.end(); ++jit) {
		for (job::task_container_type::const_iterator tit = jit->tasks[type].begin();
		     tit != jit->tasks[type].end(); ++tit) {
			points.push_back(tit->stime);
			points.push_back(-tit->ftime);
		}
	}
	if (points.size() == 0) {
		ULIB_DEBUG("no task, use default utilization 0");
		return 0;
	}
	if (points.size() & 1) {
		ULIB_FATAL("expect even number of points, got %zu", points.size());
		return -1;
	}
	heap_init_ut(&*points.begin(), &*points.end());

	double first = points.begin()->time;
	double last  = first < 0? -first: first;
	double util  = 0;
	int    in    = first < 0? -1: 1;
	heap_pop_to_rear_ut(&*points.begin(), &*points.end());
	points.pop_back();
	do {
		double t = points.begin()->time;
		double s = t < 0? -t: t;
		if ((in < 0 || in > nslots) && !double_equal(s, last))
			ULIB_FATAL("used slots(%d) is illegal between %f and %f", in, last, s);
		util += in * (s - last);
		if (t < 0)
			--in;
		else
			++in;
		last = s;
		heap_pop_to_rear_ut(&*points.begin(), &*points.end());
		points.pop_back();
	} while (points.size());

	if (in) {
		ULIB_FATAL("used slots(%d) should be zero in the end", in);
		return -1;
	}

	return util / (last - first) / nslots;
}

int import_workload(const char *file, job_tracker::pool_container_type *pools)
{
	// Sample line:
	// modeling job_201405200258_257255:HIGH task_201405200258_257255_m_021770 \
	// MAP 1403620325026 1403620325033 1403620344772

	FILE *fp = fopen(file, "r");
	if (fp == NULL) {
		ULIB_WARNING("cannot open %s for reading", file);
		return -1;
	}

	ulib::open_hash_map<uint64_t, pool *> pmap;
	ulib::open_hash_map<uint64_t, job  *> jmap;

	for (job_tracker::pool_container_type::iterator pit = pools->begin();
	     pit != pools->end(); ++pit)
		pmap[pit->id] = &*pit;

	int ret = -1;
	while (!ferror(fp) && !feof(fp)) {
		char pstr[1024];
		char jstr[1024];
		char prio[16];
		char tstr[1024];
		char type[16];
		double ctime, stime, ftime;
		if (fscanf(fp, "%[^\t]\t%[^:]:%[^\t]\t%[^\t]\t%[^\t]\t%lf\t%lf\t%lf\n",
			   pstr, jstr, prio, tstr, type, &ctime, &stime, &ftime) != 8) {
			ULIB_WARNING("Error encounterred while parsing a line");
			goto done;
		}
		uint64_t pid = pool::id_from_str(pstr);
		pool *p = pmap[pid];
		if (p == NULL) {
			ULIB_FATAL("pool %s has not been configured", pstr);
			goto done;
		}
		uint64_t jid = job::id_from_str(jstr);
		job *j = jmap[jid];
		if (j == NULL) {
			job nj;
			nj.id = jid;
			nj.ctime = ctime;
			nj.fs_ctx_map.uid = jid;
			nj.fs_ctx_reduce.uid = jid;
			j = &p->add_job(nj);
			jmap[jid] = j;
		} else if (j->ctime > ctime)
			j->ctime = ctime;
		double jw;
		if (strcmp("NORMAL", prio) == 0)
			jw = 1.0;
		else if (strcmp("HIGH", prio) == 0)
			jw = 2.0;
		else if (strcmp("VERY_HIGH", prio) == 0)
			jw = 4.0;
		else {
			ULIB_WARNING("job priority unrecognized:%s", prio);
			goto done;
		}
		j->fs_ctx_map.weight = jw;
		j->fs_ctx_reduce.weight = jw;
		task t;
		t.id = task::id_from_str(tstr);
		t.ctime = ctime;
		t.stime = stime;
		t.ftime = ftime;
		t.ptime = ftime - stime;
		t.type = strcmp("MAP", type) == 0? task::TASK_TYPE_MAP: task::TASK_TYPE_REDUCE;
		j->tasks[t.type].push_back(t);
	}

	ret = 0;
done:
	fclose(fp);
	return ret;
}

// Support ptime instead of stime and ftime
int import_workload1(const char *file, job_tracker::pool_container_type *pools)
{
	// Sample line:
	// modeling job_201405200258_257255:HIGH task_201405200258_257255_m_021770 \
	// MAP 1403620325026 1024

	FILE *fp = fopen(file, "r");
	if (fp == NULL) {
		ULIB_WARNING("cannot open %s for reading", file);
		return -1;
	}

	ulib::open_hash_map<uint64_t, pool *> pmap;
	ulib::open_hash_map<uint64_t, job  *> jmap;

	for (job_tracker::pool_container_type::iterator pit = pools->begin();
	     pit != pools->end(); ++pit)
		pmap[pit->id] = &*pit;

	int ret = -1;
	while (!ferror(fp) && !feof(fp)) {
		char pstr[1024];
		char jstr[1024];
		char prio[16];
		char tstr[1024];
		char type[16];
		double ctime, ptime;
		if (fscanf(fp, "%[^\t]\t%[^:]:%[^\t]\t%[^\t]\t%[^\t]\t%lf\t%lf\n",
			   pstr, jstr, prio, tstr, type, &ctime, &ptime) != 7) {
			ULIB_WARNING("Error encounterred while parsing a line");
			goto done;
		}
		uint64_t pid = pool::id_from_str(pstr);
		pool *p = pmap[pid];
		if (p == NULL) {
			ULIB_FATAL("pool %s has not been configured", pstr);
			goto done;
		}
		uint64_t jid = job::id_from_str(jstr);
		job *j = jmap[jid];
		if (j == NULL) {
			job nj;
			nj.id = jid;
			nj.ctime = ctime;
			nj.fs_ctx_map.uid = jid;
			nj.fs_ctx_reduce.uid = jid;
			j = &p->add_job(nj);
			jmap[jid] = j;
		} else if (j->ctime > ctime)
			j->ctime = ctime;
		double jw;
		if (strcmp("NORMAL", prio) == 0)
			jw = 1.0;
		else if (strcmp("HIGH", prio) == 0)
			jw = 2.0;
		else if (strcmp("VERY_HIGH", prio) == 0)
			jw = 4.0;
		else {
			ULIB_WARNING("job priority unrecognized:%s", prio);
			goto done;
		}
		j->fs_ctx_map.weight = jw;
		j->fs_ctx_reduce.weight = jw;
		task t;
		t.id = task::id_from_str(tstr);
		t.ctime = ctime;
		t.stime = -1;
		t.ftime = -1;
		t.ptime = ptime;
		t.type = strcmp("MAP", type) == 0? task::TASK_TYPE_MAP: task::TASK_TYPE_REDUCE;
		j->tasks[t.type].push_back(t);
	}

	ret = 0;
done:
	fclose(fp);
	return ret;
}

int export_schedule(const char *file, const job_tracker::pool_container_type &pools)
{
	FILE *fp = fopen(file, "w");
	if (fp == NULL) {
		ULIB_WARNING("cannot open %s for writing", file);
		return -1;
	}

	for (job_tracker::pool_container_type::const_iterator pit = pools.begin();
	     pit != pools.end(); ++pit) {
		for (pool::job_container_type::const_iterator jit = pit->jobs.begin();
		     jit != pit->jobs.end(); ++jit) {
			for (job::task_container_type::const_iterator tit = jit->tasks[task::TASK_TYPE_MAP].begin();
			     tit != jit->tasks[task::TASK_TYPE_MAP].end(); ++tit) {
				// pool job task type ctime ptime stime ftime
				fprintf(fp, "%s\t%016llx:%lf\t%016llx\t%d\t%f\t%f\t%f\t%f\n",
					pit->name.c_str(), jit->id, jit->fs_ctx_map.weight, tit->id,
					task::TASK_TYPE_MAP, tit->ctime, tit->ptime, tit->stime, tit->ftime);
			}
			for (job::task_container_type::const_iterator tit = jit->tasks[task::TASK_TYPE_REDUCE].begin();
			     tit != jit->tasks[task::TASK_TYPE_REDUCE].end(); ++tit) {
				// pool job task type ctime ptime stime ftime
				fprintf(fp, "%s\t%016llx:%lf\t%016llx\t%d\t%f\t%f\t%f\t%f\n",
					pit->name.c_str(), jit->id, jit->fs_ctx_reduce.weight, tit->id,
					task::TASK_TYPE_REDUCE, tit->ctime, tit->ptime, tit->stime, tit->ftime);
			}
		}
	}

	fclose(fp);

	return 0;
}

}
