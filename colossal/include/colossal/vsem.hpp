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

#ifndef _COLOSSAL_VSEM_H
#define _COLOSSAL_VSEM_H

#include <queue>

namespace colossal {

class engine;

template<typename T>
class vsem
{
public:
        vsem(int val) : _val(val) { }

	~vsem()
	{
		while (_wlist.size()) {
			delete _wlist.front();
			_wlist.pop();
		}
	}

        bool wait(T obj)
        {
                if (_val > 0) {
                        --_val;
			return true;
		}
		_wlist.push(obj);
		return false;
        }

        void post(engine *eng)
        {
		++_val;
                if (_wlist.size()) {
			T obj = _wlist.front();
                        _wlist.pop();
			(*obj)(eng);
			delete obj;
                }
        }

	size_t size() const
	{
		return _wlist.size();
	}

private:
        std::queue<T> _wlist;
        int _val;
};

}

#endif
