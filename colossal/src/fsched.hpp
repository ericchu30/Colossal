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

#ifndef _COLOSSAL_FSCHED_H
#define _COLOSSAL_FSCHED_H

#include <cstddef>
#include <stdint.h>
#include <vector>
#include <functional>
#include <ulib/heap_prot.h>
#include <ulib/hash_open.h>
#include <ulib/util_log.h>
#include "common.hpp"
#include "pointer.hpp"

namespace colossal
{

// Basic settings of a user (pool/job)
struct fs_conf {
        double weight;
        double minshare;
        int    demand;

        fs_conf() : weight(1.0), minshare(0), demand(0) { }
        fs_conf(double w, double m, int d) : weight(w), minshare(m), demand(d) { }

        virtual
        ~fs_conf() { }
};

// User settings and status
struct fs_context : public fs_conf {
        double   fairshare;
        int      alloc;
        uint64_t uid; // unique user id used for stable sorting

        fs_context() : fairshare(0), alloc(0), uid(0) { }
        fs_context(double w, double m, int d, uint64_t u = 0)
                : fs_conf(w, m, d), fairshare(0), alloc(0), uid(u) { }

        operator size_t() const
        {
                return uid;
        }

	// Fair share comparator
        int  operator()(const fs_context &a, const fs_context &b) const;

        bool operator< (const fs_context &other) const
        {
                return this->operator()(*this, other) < 0;
        }

        bool operator> (const fs_context &other) const
        {
                return this->operator()(*this, other) > 0;
        }
};

// Fair share computation using the given ratio
// Returns the computed fair share
static inline double compute_fairshare(const fs_conf &conf, double r)
{
        return std::min((double)conf.demand,
                        std::max(conf.weight * r, conf.minshare));
}

// Scale the min shares so that they add up to at most the total number of slots
// T is an iterator of a class that inherits fs_conf
// total: the total number of slots of certain type (map/reduce)
template<typename T>
static void scale_minshares(T begin, T end, int total)
{
        double sum = 0;
        for (T it = begin; it != end; ++it)
                sum += ((fs_conf *)it)->minshare;
        if (sum > total) {
                double r = total / sum;
                for (T it = begin; it != end; ++it)
                        ((fs_conf *)it)->minshare *= r;
        }
}

// The function computes the fair share for each fs_context instance
// T is an iterator of a class that inherits fs_context
// total: the total number of slots of certain type (map/reduce)
// Returns the fair share ratio
// Note: must first scale the min shares
template<typename T>
static double compute_fairshares(T begin, T end, int total)
{
        double ru = 1.0;

        // determine the upper bound for r
        for (;; ru *= 2) {
                double sum = 0;
                bool  good = true;
                for (T it = begin; it != end; ++it) {
                        double fs = compute_fairshare(*it, ru);
                        if (fs < ((fs_context *)it)->demand)
                                good = false;
                        sum += fs;
                }
                if (good || sum >= total)
                        break;
        }

        int nloop = 25;
        double sum;
        for (double rl = 0; nloop-- > 0;) {
                double m = (rl + ru) / 2.0;
                bool good = true;
                sum = 0;
                for (T it = begin; it != end; ++it) {
                        ((fs_context *)it)->fairshare = compute_fairshare(*it, m);
                        if (((fs_context *)it)->fairshare < ((fs_context *)it)->demand)
                                good = false;
                        sum += ((fs_context *)it)->fairshare;
                }
                if (good || sum >= total)
                        ru = m;
                else
                        rl = m;
        }

        return ru;
}

// The function computes the fair share ratio
// T is an iterator of a class that inherits fs_conf
// total: the total number of slots of certain type (map/reduce)
// Returns the fair share ratio
// Note: must first scale the min shares
template<typename T>
static double compute_ratio(T begin, T end, int total)
{
        double ru = 1.0;

        // determine the upper bound for r
        for (;; ru *= 2) {
                double sum = 0;
                bool  good = true;
                for (T it = begin; it != end; ++it) {
                        double fs = compute_fairshare(*it, ru);
                        if (fs < ((fs_conf *)it)->demand)
                                good = false;
                        sum += fs;
                }
                if (good || sum >= total)
                        break;
        }

        int nloop = 25;
        double sum;
        for (double rl = 0; nloop-- > 0;) {
                double m = (rl + ru) / 2.0;
                bool good = true;
                sum = 0;
                for (T it = begin; it != end; ++it) {
                        double fs = compute_fairshare(*it, m);
                        if (fs < ((fs_conf *)it)->demand)
                                good = false;
                        sum += fs;
                }
                if (good || sum >= total)
                        ru = m;
                else
                        rl = m;
        }

        return ru;
}

// Given a set of users (specified by start and end iterators), this
// class selects tasks one at a time from the users using fair
// scheduling and updates the allocation accordingly.
// Note:
//   1) The calling function should check the availability of free
//      slots.
//   2) The demand of each user can be adjusted from outside; however,
//      the invariant demand >= allocation has to be guaranteed.
//   3) Once a user's demand is met (allocation == demand), it will be
//      opted from future consideration even if the demand is
//      raised. In this case, the user should be added back via add().
template<typename T>
class fs_select
{
public:
        DEFINE_HEAP(inclass, T, std::greater< comp_pointer<T> >());

        fs_select(T begin, T end)
                : _begin(begin), _end(end)
        {
                for (T it = begin; it != end; ++it) {
                        _heap.push_back(it);
                        _users.insert(it);
                }
                heap_init_inclass(&*_heap.begin(), &*_heap.end());
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
