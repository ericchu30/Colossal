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

#ifndef _COLOSSAL_JOB_H
#define _COLOSSAL_JOB_H

#include <string>
#include <vector>
#include "task.hpp"
#include "fsched.hpp"

namespace colossal
{

struct job
{
	typedef std::vector<task> task_container_type;

        uint64_t   id;
	double     ctime;
	fs_context fs_ctx_map;
	fs_context fs_ctx_reduce;
        task_container_type tasks[task::TASK_TYPE_NUM];

	static uint64_t id_from_str(const char *str);

        std::string to_str(const char *prefix = "") const;
};

}

#endif
