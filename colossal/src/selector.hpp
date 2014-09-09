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

#ifndef _COLOSSAL_SELECTOR_H
#define _COLOSSAL_SELECTOR_H

#include <cstddef>
#include <stdint.h>
#include <list>
#include <queue>
#include <vector>
#include <ulib/heap_prot.h>
#include <ulib/hash_open.h>
#include <ulib/math_rand_prot.h>
#include "common.hpp"
#include "hashable.hpp"
#include "comparable.hpp"
#include "task.hpp"
#include "job.hpp"
#include "pool.hpp"
#include "fsched.hpp"

namespace colossal {

class selector
{
public:
	struct pool_view {
		td_ref *ptr;

		typedef td_ref * pointer_type;

		pool_view(td_ref * p) : ptr(p) { }

		operator td_ref *&()
		{
			return ptr;
		}

		operator size_t() const
		{
			return ptr->getpool()->id;
		}

		bool operator==(const pool_view &other) const
		{
			return ptr->getpool()->id == other.ptr->getpool()->id;
		}
	};

	struct job_view {
		td_ref * ptr;

		typedef td_ref * pointer_type;

		job_view(td_ref * p) : ptr(p) { }

		operator td_ref *&()
		{
			return ptr;
		}

		operator size_t() const
		{
			uint64_t h = ptr->getjob()->id;
			return RAND_INT_MIX64(h) + ptr->getpool()->id;
		}

		bool operator==(const job_view &other) const
		{
			return ptr->getjob()->id == other.ptr->getjob()->id &&
				ptr->getpool()->id == other.ptr->getpool()->id;
		}
	};

	typedef std::list<pool>::iterator pool_itr_type;
	typedef ulib::open_hash_set<pool_view> changes_type;
	typedef ulib::open_hash_map<job_view, std::queue<td_ref *> *> j2t_type;
        typedef ulib::open_hash_map<pool_view, j2t_type *> p2j_type;

	DEFINE_HEAP(inclass, td_ref *, std::greater<ctime_comp>());

	struct pool_map_fs_itr {
		p2j_type::iterator itr;

		pool_map_fs_itr() { }
		pool_map_fs_itr(const p2j_type::iterator &it) : itr(it) { }

		pool_map_fs_itr &operator++()
		{
			++itr;
			return *this;
		}

		bool operator==(const pool_map_fs_itr &other) const
		{
			return itr == other.itr;
		}

		bool operator!=(const pool_map_fs_itr &other) const
		{
			return itr != other.itr;
		}

		const fs_context & operator *() const
		{
			return ((td_ref *)itr.key())->getpool()->fs_ctx_map;
		}

		operator fs_context *()
		{
			return &((td_ref *)itr.key())->getpool()->fs_ctx_map;
		}
	};

	struct job_map_fs_itr {
		j2t_type::iterator itr;

		job_map_fs_itr() { }
		job_map_fs_itr(const j2t_type::iterator &it) : itr(it) { }

		job_map_fs_itr &operator++()
		{
			++itr;
			return *this;
		}

		bool operator==(const job_map_fs_itr &other) const
		{
			return itr == other.itr;
		}

		bool operator!=(const job_map_fs_itr &other) const
		{
			return itr != other.itr;
		}

		const fs_context & operator *() const
		{
			return ((td_ref *)itr.key())->getjob()->fs_ctx_map;
		}

		operator fs_context *()
		{
			return &((td_ref *)itr.key())->getjob()->fs_ctx_map;
		}
	};

	struct job_map_fcfs_itr {
		j2t_type::iterator itr;

		job_map_fcfs_itr() { }
		job_map_fcfs_itr(const j2t_type::iterator &it) : itr(it) { }

		job_map_fcfs_itr &operator++()
		{
			++itr;
			return *this;
		}

		bool operator==(const job_map_fcfs_itr &other) const
		{
			return itr == other.itr;
		}

		bool operator!=(const job_map_fcfs_itr &other) const
		{
			return itr != other.itr;
		}

		const job_ctime_hash operator *() const
		{
			return (td_ref *)itr.key();
		}

		operator fs_context *()
		{
			return &((td_ref *)itr.key())->getjob()->fs_ctx_map;
		}
	};

	struct pool_reduce_fs_itr {
		p2j_type::iterator itr;

		pool_reduce_fs_itr() { }
		pool_reduce_fs_itr(const p2j_type::iterator &it) : itr(it) { }

		pool_reduce_fs_itr &operator++()
		{
			++itr;
			return *this;
		}

		bool operator==(const pool_reduce_fs_itr &other) const
		{
			return itr == other.itr;
		}

		bool operator!=(const pool_reduce_fs_itr &other) const
		{
			return itr != other.itr;
		}

		const fs_context & operator *() const
		{
			return ((td_ref *)itr.key())->getpool()->fs_ctx_reduce;
		}

		operator fs_context *()
		{
			return &((td_ref *)itr.key())->getpool()->fs_ctx_reduce;
		}
	};

	struct job_reduce_fs_itr {
		j2t_type::iterator itr;

		job_reduce_fs_itr() { }
		job_reduce_fs_itr(const j2t_type::iterator &it) : itr(it) { }

		job_reduce_fs_itr &operator++()
		{
			++itr;
			return *this;
		}

		bool operator==(const job_reduce_fs_itr &other) const
		{
			return itr == other.itr;
		}

		bool operator!=(const job_reduce_fs_itr &other) const
		{
			return itr != other.itr;
		}

		const fs_context & operator *() const
		{
			return ((td_ref *)itr.key())->getjob()->fs_ctx_reduce;
		}

		operator fs_context *()
		{
			return &((td_ref *)itr.key())->getjob()->fs_ctx_reduce;
		}
	};

	struct job_reduce_fcfs_itr {
		j2t_type::iterator itr;

		job_reduce_fcfs_itr() { }
		job_reduce_fcfs_itr(const j2t_type::iterator &it) : itr(it) { }

		job_reduce_fcfs_itr &operator++()
		{
			++itr;
			return *this;
		}

		bool operator==(const job_reduce_fcfs_itr &other) const
		{
			return itr == other.itr;
		}

		bool operator!=(const job_reduce_fcfs_itr &other) const
		{
			return itr != other.itr;
		}

		const job_ctime_hash operator *() const
		{
			return (td_ref *)itr.key();
		}

		operator fs_context *()
		{
			return &((td_ref *)itr.key())->getjob()->fs_ctx_reduce;
		}
	};

	selector(const pool_itr_type &pb, const pool_itr_type &pe);
	~selector();

	// preempted tasks may need to be added back
	void add_preempted_map(td_ref *ref);
	void add_preempted_reduce(td_ref *ref);

	double map_min_ctime() const;
	double reduce_min_ctime() const;

	size_t maps_popped() const { return _maps_popped; }
	size_t maps_seen() const { return _seen_maps.size(); }
	size_t maps_left() const { return _map_refs.size(); }
	size_t reduces_popped() const { return _reduces_popped; }
	size_t reduces_seen() const { return _seen_reduces.size(); }
	size_t reduces_left() const { return _reduce_refs.size(); }

	bool has_map() const { return _map_refs.size() || _maps_popped < _seen_maps.size(); }
	bool has_reduce() const { return _reduce_refs.size() || _reduces_popped < _seen_reduces.size(); }
	bool has_task() const { return has_map() || has_reduce(); }

	void dump_seen_task_tree() const;

	// update the visibility of maps/reduces to the scheduler
	void see_maps(double now, changes_type *changes = NULL);
	void see_reduces(double now, changes_type *changes = NULL);

	// pop out a map/reduce task
	// Note: popped tasks should NOT be freed from outside
	td_ref *pop_map();  // pop only
	td_ref *pop_map(double now); // see and pop
	td_ref *pop_reduce();  // pop only
	td_ref *pop_reduce(double now);  // see and pop

private:
	static td_ref * job_select_map_fs(pool_map_fs_itr pchosen);
	static td_ref * job_select_map_fcfs(pool_map_fs_itr pchosen);
	static td_ref * job_select_reduce_fs(pool_reduce_fs_itr pchosen);
	static td_ref * job_select_reduce_fcfs(pool_reduce_fs_itr pchosen);

	pool_itr_type _pb;
	pool_itr_type _pe;
	p2j_type _map_tasks;
	p2j_type _reduce_tasks;
	size_t _maps_popped;     // maps popped out by now
	size_t _reduces_popped;  // reduces popped out by now
	std::vector<td_ref *> _map_refs;
	std::vector<td_ref *> _seen_maps;
	std::vector<td_ref *> _reduce_refs;
	std::vector<td_ref *> _seen_reduces;
};

}

#endif
