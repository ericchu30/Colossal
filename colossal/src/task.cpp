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
#include <cstring>
#include <ulib/hash_func.h>
#include <ulib/math_rand_prot.h>
#include "job.hpp"
#include "pool.hpp"
#include "task.hpp"

namespace colossal
{

uint64_t task::id_from_str(const char *str)
{
	return hash_fast64(str, strlen(str), 0xdeedbeefdeedbeefull);
}

std::string task::to_str() const
{
        char buf[1024];

        snprintf(buf, sizeof(buf), "%016llx,%f,%f,%f,%f,%s",
                 id, ctime, ptime, stime, ftime,
		 type == TASK_TYPE_MAP? "MAP": "REDUCE");

        return buf;
}

task_desc::ref::ref(task_desc *p)
        : _td(p)
{
        ++_td->_refcnt;
}

task_desc::ref::ref(const task_desc::ref &other)
{
        _td = other._td;
        ++_td->_refcnt;
}

task_desc::ref::~ref()
{
        --_td->_refcnt;
        if (_td->_refcnt == 0)
                delete _td;
}

task_desc::ref &task_desc::ref::operator= (const task_desc::ref &other)
{
        if (_td != other._td) {
                --_td->_refcnt;
                if (_td->_refcnt == 0)
                        delete _td;
                _td = other._td;
                ++_td->_refcnt;
        }
        return *this;
}

task_desc::ref::operator size_t() const
{
	uint64_t h = _td->_task->id;
	h = RAND_INT_MIX64(h) + _td->_job->id;
	h = RAND_INT3_MIX64(h) + _td->_pool->id;
	return RAND_INT3_MIX64(h);
}

bool task_desc::ref::operator==(const task_desc::ref &other) const
{
	return _td->_task->id == other._td->_task->id &&
		_td->_job->id == other._td->_job->id &&
		_td->_pool->id == other._td->_pool->id;
}

}
