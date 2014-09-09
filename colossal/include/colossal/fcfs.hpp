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

#ifndef _COLOSSAL_FCFS_H
#define _COLOSSAL_FCFS_H

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <functional>
#include <ulib/heap_prot.h>
#include <ulib/hash_open.h>
#include <ulib/util_log.h>
#include "comparable.hpp"
#include "common.hpp"

namespace colossal
{

// Given a set of users (specified by start and end iterators), this
// class selects tasks one at a time from the users using FCFS policy
// and updates the allocation accordingly.
// Note:
//   1) The calling function should check the availability of free
//      slots.
//   2) The demand of each user can be adjusted from outside; however,
//      the invariant demand >= allocation has to be guaranteed.
//   3) Once a user's demand is met (allocation == demand), it will be
//      opted from future consideration even if the demand is
//      raised. In this case, the user should be added back via add().
template<typename T>
class fcfs_select
{
public:
        DEFINE_HEAP(inclass, T, std::greater<ctime_comp>());

        fcfs_select(T begin, T end)
                : _begin(begin), _end(end)
        {
                for (T it = begin; it != end; ++it) {
                        _heap.push_back(it);
                        _users.insert(it);
                }
                heap_init_inclass(&*_heap.begin(), &*_heap.end());
//                ULIB_DEBUG("%llu unique users", _users.size());
        }

        bool add(T it)
        {
                if (((fs_context *)it)->demand == ((fs_context *)it)->alloc)
                        return false;
                if (_users.contain(it))
                        return false;
                _users.insert(it);
                _heap.push_back(it);
                heap_push_inclass(&*_heap.begin(), _heap.size() - 1, 0, it);
                return true;
        }

	// get the number of ready tasks
	size_t task_count() const
	{
		return _heap.size();
	}

	// get the number of unique users
	size_t user_count() const
	{
		return _users.size();
	}

        // Selects a task and returns its iterator (of type T)
        // When there is no task left, returns @_end
        T operator()()
        {
                T top;
                for (; _heap.size();) {
                        top = *_heap.begin();
                        if (((fs_context *)top)->demand == ((fs_context *)top)->alloc) {
                                _users.erase(top);
                                heap_pop_to_rear_inclass(&*_heap.begin(), &*_heap.end());
                                _heap.pop_back();
                        } else
                                break;
                }
                if (_heap.size()) {
                        ++((fs_context *)(*_heap.begin()))->alloc;
                        heap_adjust_inclass(&*_heap.begin(), _heap.size(), 0, *_heap.begin());
                        return top;
                }
                return _end;
        }

private:
        T _begin;
        T _end;
        std::vector<T> _heap;
        ulib::open_hash_set<hash_pointer<T>, except> _users;
};

}

#endif
