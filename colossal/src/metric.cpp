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
#include "metric.hpp"

namespace colossal
{

metric metric::operator[](const std::string &key)
{
	metric m = *this;
	if (_key.size())
		m.set_key(_key + "\t" + key);
	else
		m.set_key(key);
	return m;
}

void metric::set_value(double val)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%lf", val);
	set_value(buf);
}

void metric::set_value(int val)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d", val);
	set_value(buf);
}

void metric::set_value(uint64_t val)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%llu", val);
	set_value(buf);
}

}
