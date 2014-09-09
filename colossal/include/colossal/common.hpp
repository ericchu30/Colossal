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

#ifndef _COLOSSAL_COMMON_H
#define _COLOSSAL_COMMON_H

#include <cstddef>
#include <exception>

namespace colossal
{

// Timestamp precision
static const double PRECISION = 0.000001;

// Exception throwed by the library
struct except : public std::exception
{
        virtual	~except() throw() { }
};

static inline bool
double_equal(double a, double b,
	     double epsilon = PRECISION)
{
	double d = a - b;
	return d < epsilon && d > -epsilon;
}

}

#endif
