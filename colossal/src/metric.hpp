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

#ifndef _COLOSSAL_METRIC_H
#define _COLOSSAL_METRIC_H

#include <cstdio>
#include <stdint.h>
#include <string>

namespace colossal
{

class metric {
public:
	metric(const std::string pref = "", FILE * fp = stdout)
		: _key(pref), _fp(fp) { }

	void set_key(const std::string &key)
	{
		_key = key;
	}

	void set_value(const std::string &val)
	{
		fprintf(_fp, "%s\t%s\n", _key.c_str(), val.c_str());
	}

	void set_value(double val);
	void set_value(int val);
	void set_value(uint64_t val);

	metric operator[](const std::string &key);

protected:
	std::string _key;
	FILE *_fp;
};

}

#endif
