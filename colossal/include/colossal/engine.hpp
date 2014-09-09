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

#ifndef _COLOSSAL_ENGINE_H
#define _COLOSSAL_ENGINE_H

#include <list>
#include <vector>
#include <string>
#include "hlist.hpp"
#include "pointer.hpp"
#include "hashable.hpp"
#include "vsem.hpp"
#include "task.hpp"
#include "pool.hpp"
#include "event.hpp"
#include "selector.hpp"

namespace colossal
{

template<typename T>
struct map_fs_itr {
	typename T::iterator itr;

	map_fs_itr(const typename T::iterator &it) : itr(it) { }

	map_fs_itr &operator++()
	{
		++itr;
		return *this;
	}

	bool operator==(const map_fs_itr &other) const
	{
		return itr == other.itr;
	}

	bool operator!=(const map_fs_itr &other) const
	{
		return itr != other.itr;
	}

	operator fs_context *()
	{
		return &itr->fs_ctx_map;
	}
};

template<typename T>
struct reduce_fs_itr {
	typename T::iterator itr;

	reduce_fs_itr(const typename T::iterator &it) : itr(it) { }

	reduce_fs_itr &operator++()
	{
		++itr;
		return *this;
	}

	bool operator==(const reduce_fs_itr &other) const
	{
		return itr == other.itr;
	}

	bool operator!=(const reduce_fs_itr &other) const
	{
		return itr != other.itr;
	}

	operator fs_context *()
	{
		return &itr->fs_ctx_reduce;
	}
};

class engine
{
public:
	DEFINE_HEAP(inclass, event *, std::greater< comp_pointer<event *> >());

        static const double LOAD_FACTOR;
	static const int    PROGRESS_WINSIZE;

        typedef hlist<stime_hash>    taskset_type;
        typedef std::vector<event *> eventheap_type;
	typedef std::list<pool>      pool_container_type;
	typedef vsem<event *>        vsem_type;

        engine(int nmaps,        // number of map slots in the cluster
	       int nreduces,     // number of reduce slots in the cluster
	       double now = 0);  // job_tracker boot time

        ~engine();

	// Set the output metric file and metric sampling window size
	bool set_metrics(const char * met, int met_win);

	// Add a pool to the engine
	pool &add_pool(const std::string &ns, double mto, double fto,
		       double weight, int minmap, int minred,
		       pool::sched_mode sched);

	// Scale map and reduce min shares
	// Required if pool min shares exceed the maximum number of slots
	void scale_minshares();

	// Start processing jobs in the pools
        void process();

	const pool_container_type &getpools() const { return _pools; }
	pool_container_type &getpools() { return _pools; }

	// Event APIs
	void add_event(event *ev);
	void run_map(td_ref *t);
	void run_reduce(td_ref *t);
	void finish_map(td_ref *t);
	void finish_reduce(td_ref *t);
        void preempt_maps(int num);
        void preempt_reduces(int num);
	void update_map_fairshares();
	void update_reduce_fairshares();

	double        time_now;
        vsem_type    *sem_map;
        vsem_type    *sem_reduce;
        taskset_type *running_maps;
        taskset_type *running_reduces;
	selector     *select;

private:
        void   submit_tasks();
	double map_progress() const;
	double reduce_progress() const;

        pool_container_type _pools;
        eventheap_type _eventheap;
        int _nmap;
        int _nreduce;
	int _met_win;
	FILE * _fp_met;
};

}

#endif
