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

#include <algorithm>
#include <ulib/util_algo.h>
#include "fsched.hpp"

namespace colossal
{

int fs_context::operator()(const fs_context &a, const fs_context &b) const
{
        int ret;
        int m1 = std::min((int)a.minshare, a.demand);
        int m2 = std::min((int)b.minshare, b.demand);
        bool needy1 = a.alloc < m1;
        bool needy2 = b.alloc < m2;
        double sr1 = a.alloc / std::max(a.minshare, 1.0);
        double sr2 = b.alloc / std::max(b.minshare, 1.0);
        double wr1 = a.alloc / a.weight;
        double wr2 = b.alloc / b.weight;
        if (needy1 && !needy2)
                ret = -1;
        else if (!needy1 && needy2)
                ret = 1;
        else if (needy1 && needy2)
                ret = generic_compare(sr1, sr2);
        else
                ret = generic_compare(wr1, wr2);
        if (ret == 0)
                return generic_compare(a.uid, b.uid);
        return ret;
}

}
