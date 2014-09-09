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

#ifndef _COLOSSAL_HASHABLE_H
#define _COLOSSAL_HASHABLE_H

#include <cstddef>
#include "task.hpp"

namespace colossal
{

struct stime_hash {
	td_ref * ptr;

	typedef td_ref * pointer_type;

	stime_hash(td_ref * p) : ptr(p) { }

	operator td_ref *&()
	{
		return ptr;
	}

	operator size_t() const
	{
		return *ptr;
	}

	// sort in the order of reversed task start time
	bool operator> (const stime_hash &other) const
	{
		return ptr->gettask()->stime < other.ptr->gettask()->stime;
	}

	// sort in the order of reversed task start time
	bool operator< (const stime_hash &other) const
	{
		return ptr->gettask()->stime > other.ptr->gettask()->stime;
	}

        bool operator==(const stime_hash &other) const
        {
                return *ptr == *other.ptr;
        }
};

struct job_ctime_hash {
	td_ref * ptr;

	typedef td_ref * pointer_type;

	job_ctime_hash(td_ref * p) : ptr(p) { }

	operator td_ref *&()
	{
		return ptr;
	}

	operator size_t() const;

	// sort by job ctime and priority
	bool operator> (const job_ctime_hash &other) const;
	bool operator< (const job_ctime_hash &other) const;

        bool operator==(const job_ctime_hash &other) const;
};

}

#endif
