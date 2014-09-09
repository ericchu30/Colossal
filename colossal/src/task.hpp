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

#ifndef _COLOSSAL_TASK_H
#define _COLOSSAL_TASK_H

#include <stdint.h>
#include <string>

namespace colossal
{

struct job;
struct pool;

struct task
{
        enum task_type {
                TASK_TYPE_REDUCE = 0,
                TASK_TYPE_MAP,
                TASK_TYPE_NUM
        };

	enum task_flag {
		TASK_FLAG_NONE      = 0,
		TASK_FLAG_POPPED    = 1,
		TASK_FLAG_PREEMPTED = 2
	};

	static uint64_t id_from_str(const char *str);

        std::string to_str() const;

        uint64_t id;
        double ctime;  // creation time
        double ptime;  // processing time
        double stime;  // start time
        double ftime;  // finish time
        task_type type;
};

// Reference-counted task description class
// Should only be instantiated using new, and then access the instance
// using the ref member class
class task_desc
{
public:
        class ref
        {
        public:
                ref(task_desc *p);
                ref(const ref &other);
                virtual ref &operator= (const ref &other);

                virtual ~ref();

		void set_flag(task::task_flag flag)
		{
			_td->_flags |= flag;
		}

		void clear_flag()
		{
			_td->_flags = 0;
		}

		void clear_flag(task::task_flag flag)
		{
			_td->_flags &= ~flag;
		}

		bool test_flag(task::task_flag flag)
		{
			return _td->_flags & flag;
		}

                const task *gettask() const
                {
                        return _td->_task;
                }

                const job  *getjob() const
                {
                        return _td->_job;
                }

                const pool *getpool() const
                {
                        return _td->_pool;
                }

                task *gettask()
                {
                        return _td->_task;
                }

                job  *getjob()
                {
                        return _td->_job;
                }

                pool *getpool()
                {
                        return _td->_pool;
                }

		operator size_t() const;

		bool operator==(const ref &other) const;

        private:
                task_desc *_td;
        };

        friend class ref;

        task_desc(task *t, job *j, pool *p)
		: _task(t), _job(j), _pool(p),
		  _refcnt(0), _flags(0) { }

        virtual ~task_desc() { }

private:
        task *_task;
        job  *_job;
        pool *_pool;
        int   _refcnt;
	unsigned int _flags;
};

typedef task_desc::ref td_ref;

}

#endif
