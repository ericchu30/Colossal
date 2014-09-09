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

#ifndef _COLOSSAL_LOG_H
#define _COLOSSAL_LOG_H

#include <cstdio>

#ifdef NDEBUG
#define DEBUG(time, fmt, ...)
#else
#define DEBUG(time, fmt, ...)						\
	fprintf(stdout, "[D] @%f\t" fmt "\n", time, ##__VA_ARGS__)
#endif

#define NOTICE(time, fmt, ...)						\
	fprintf(stdout, "[I] @%f\t" fmt "\n", time, ##__VA_ARGS__)

#define WARNING(time, fmt, ...)						\
	fprintf(stderr, "[W] @%f\t" fmt "\n", time, ##__VA_ARGS__)

#define FATAL(time, fmt, ...)						\
	fprintf(stderr, "[E] @%f\t" fmt "\n", time, ##__VA_ARGS__)

#endif
