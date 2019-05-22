/*
 * Object Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#include "assert.hpp"
#include "buddy.hpp"
#include "space_obj.hpp"
#include "stdio.hpp"

Capability *Space_obj::walk (unsigned long sel)
{
    unsigned l = lev;

    for (Capability **e = &root;; e = reinterpret_cast<Capability **>(*e) + (sel >> (--l * bpl) & ((1UL << bpl) - 1))) {

//        trace (0, "  %s: l=%u e=%p", __func__, l, e);

        if (!l)
            return reinterpret_cast<Capability *>(e);

        // No cap table yet, allocate one
        if (!*e) {
            *e = static_cast<Capability *>(Buddy::allocator.alloc (0, Buddy::FILL_0));
//            trace (0, "    ==> l=%u e=%p => %p allocated", l - 1, e, *e);
        }
    }
}

Capability Space_obj::lookup (unsigned long sel)
{
    Capability *ptr = walk (sel);

    return ptr ? *ptr : Capability (0);
}

void Space_obj::update (unsigned long sel, Capability cap)
{
//    trace (0, "%s: sel=%lu", __func__, sel);

    Capability *ptr = walk (sel);
    assert (ptr);

//    trace (0, "%s: got ptr=%p", __func__, ptr);

    // XXX: Handle the old capability
    Capability::exchange (ptr, cap);
}

bool Space_obj::insert (unsigned long sel, Capability cap)
{
//    trace (0, "%s: sel=%lu", __func__, sel);

    Capability *ptr = walk (sel);
    assert (ptr);

//    trace (0, "%s: got ptr=%p", __func__, ptr);

    return Capability::compare_exchange (ptr, Capability (0), cap);
}
