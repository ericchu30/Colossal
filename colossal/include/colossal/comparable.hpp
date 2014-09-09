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

#ifndef _COLOSSAL_COMPARABLE_H
#define _COLOSSAL_COMPARABLE_H

#include "task.hpp"

namespace colossal {

// Wrapper class for td_ref providing ctime comparability
struct ctime_comp {
	td_ref *ptr;

	typedef td_ref * pointer_type;

	ctime_comp(td_ref * p) : ptr(p) { }

	operator td_ref *&()
	{
		return ptr;
	}

	bool operator> (const ctime_comp &other) const
	{
		return ptr->gettask()->ctime > other.ptr->gettask()->ctime;
	}

	bool operator< (const ctime_comp &other) const
	{
		return ptr->gettask()->ctime < other.ptr->gettask()->ctime;
	}
};

}

#endif
