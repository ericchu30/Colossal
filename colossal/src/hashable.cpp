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

#include <stdint.h>
#include <ulib/math_rand_prot.h>
#include "job.hpp"
#include "pool.hpp"
#include "common.hpp"
#include "hashable.hpp"

namespace colossal
{

job_ctime_hash::operator size_t() const
{
	uint64_t h = ptr->getjob()->id;
	return RAND_INT_MIX64(h) + ptr->getpool()->id;
}

bool job_ctime_hash::operator> (const job_ctime_hash &other) const
{
	if (double_equal(ptr->getjob()->ctime, other.ptr->getjob()->ctime)) {
		// maps and reduces of a job have the same priority(weight)
		if (double_equal(ptr->getjob()->fs_ctx_map.weight,
				 other.ptr->getjob()->fs_ctx_map.weight))
			return ptr->getjob()->id > other.ptr->getjob()->id; // stable sorting
		return ptr->getjob()->fs_ctx_map.weight > other.ptr->getjob()->fs_ctx_map.weight;
	}
	return ptr->getjob()->ctime > other.ptr->getjob()->ctime;
}

bool job_ctime_hash::operator< (const job_ctime_hash &other) const
{
	if (double_equal(ptr->getjob()->ctime, other.ptr->getjob()->ctime)) {
		// maps and reduces of a job have the same priority(weight)
		if (double_equal(ptr->getjob()->fs_ctx_map.weight,
				 other.ptr->getjob()->fs_ctx_map.weight))
			return ptr->getjob()->id < other.ptr->getjob()->id;
		return ptr->getjob()->fs_ctx_map.weight < other.ptr->getjob()->fs_ctx_map.weight;
	}
	return ptr->getjob()->ctime < other.ptr->getjob()->ctime;
}

bool job_ctime_hash::operator==(const job_ctime_hash &other) const
{
	return ptr->getjob()->id == other.ptr->getjob()->id &&
		ptr->getpool()->id == other.ptr->getpool()->id;
}

}
