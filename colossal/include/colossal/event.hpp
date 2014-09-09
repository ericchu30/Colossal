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

#ifndef _COLOSSAL_EVENT_H
#define _COLOSSAL_EVENT_H

#include <ulib/util_log.h>
#include "pool.hpp"
#include "selector.hpp"

namespace colossal
{

class engine;

// Event interface class
class event
{
public:
        virtual ~event() { }

        double gettime() const
        {
                return _time;
        }

        virtual bool operator< (const event &other) const
        {
                return _time < other._time;
        }

        virtual bool operator> (const event &other) const
        {
                return _time > other._time;
        }

	// Event handler routine.
	// Returns true if the event is done, notifying the engine to
	// reclaim the memory.
        virtual bool operator()(engine *eng)
	{
		// Should be overwritten by inherited classes
		(void) eng;
		ULIB_FATAL("shouldn't have called the abstract event handler");
		return true;
	}

protected:
        double _time;
};

class ev_create_map : public event
{
public:
        ev_create_map(selector *sel);
        bool operator()(engine *eng);

private:
	selector *_sel;
};

class ev_create_reduce : public event
{
public:
	ev_create_reduce(selector *sel);
	bool operator()(engine *eng);

private:
	selector *_sel;
};

class ev_finish_map : public event
{
public:
	ev_finish_map(td_ref *ref);
	bool operator()(engine *eng);

private:
	td_ref *_ref;
};

class ev_finish_reduce : public event
{
public:
	ev_finish_reduce(td_ref *ref);
	bool operator()(engine *eng);

private:
	td_ref *_ref;
};

class ev_preempt_map : public event
{
public:
	ev_preempt_map(pool *p, double deadline);
	bool operator()(engine *eng);

private:
	pool *_pool;
};

class ev_preempt_reduce : public event
{
public:
	ev_preempt_reduce(pool *p, double deadline);
	bool operator()(engine *eng);

private:
	pool *_pool;
};

}

#endif
