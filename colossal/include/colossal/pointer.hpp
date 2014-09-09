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

#ifndef _COLOSSAL_POINTER_H
#define _COLOSSAL_POINTER_H

namespace colossal {

// The following structures are made of POD types on purpose due to
// the limitation of hlist and open_hash_map/set in ulib.
// Violation of this rule may result in segmentation faults.

// Comparable pointer to an object of type *T
template<typename T>
struct comp_pointer {
        T ptr;

	typedef T pointer_type;

        comp_pointer(T p) : ptr(p) { }

        operator T&()
        {
                return ptr;
        }

        bool operator> (const comp_pointer &other) const
        {
                return *ptr > *other.ptr;
        }

        bool operator< (const comp_pointer &other) const
        {
                return *ptr < *other.ptr;
        }
};

// Hashable pointer to an object of type *T
template<typename T>
struct hash_pointer {
        T ptr;

	typedef T pointer_type;

        hash_pointer(T p) : ptr(p) { }

        operator T&()
        {
                return ptr;
        }

        operator size_t() const
        {
                return *ptr;
        }

        bool operator==(const hash_pointer &other) const
        {
                return *ptr == *other.ptr;
        }
};

// Comparable and hashable pointer to an object of type *T
template<typename T>
struct full_pointer {
        T ptr;

	typedef T pointer_type;

        full_pointer(T p) : ptr(p) { }

        operator T&()
        {
                return ptr;
        }

        operator size_t() const
        {
                return *ptr;
        }

	// faster equality check than using < and > operators
	bool operator==(const full_pointer &other) const
        {
                return *ptr == *other.ptr;
        }

        bool operator> (const full_pointer &other) const
        {
                return *ptr > *other.ptr;
        }

        bool operator< (const full_pointer &other) const
        {
                return *ptr < *other.ptr;
        }
};

}

#endif
