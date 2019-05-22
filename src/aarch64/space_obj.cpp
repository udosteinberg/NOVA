/*
 * Object Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

Capability *Space_obj::walk (unsigned long sel, bool a)
{
    auto l = lev;

    for (Capability **e = &root, *cte;; e = reinterpret_cast<decltype (e)>(cte) + (sel >> (--l * bpl) & (BIT (bpl) - 1))) {

        if (!l)
            return reinterpret_cast<Capability *>(e);

        __atomic_load (e, &cte, __ATOMIC_RELAXED);

        if (!cte) {

            if (!a)
                return nullptr;

            auto tmp = static_cast<Capability *>(Buddy::alloc (0, Buddy::Fill::BITS0));
            assert (tmp);

            // A compare_exchange failure changes cte from 0 to the existing value at e and we continue with that
            if (!__atomic_compare_exchange (e, &cte, &tmp, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
                Buddy::free (tmp);

            else
                cte = tmp;
        }
    }
}

Capability Space_obj::lookup (unsigned long sel) const
{
    auto l = lev;

    for (Capability * const *e = &root, *cte;; e = reinterpret_cast<decltype (e)>(cte) + (sel >> (--l * bpl) & (BIT (bpl) - 1))) {

        __atomic_load (e, &cte, __ATOMIC_RELAXED);

        if (!l || !cte)
            return Capability (reinterpret_cast<uintptr_t>(cte));
    }
}

Capability Space_obj::update (unsigned long sel, Capability cap)
{
    Capability *cte = walk (sel, cap.prm()), old (0);

    if (cte)
        __atomic_exchange (cte, &cap, &old, __ATOMIC_SEQ_CST);

    return old;
}

bool Space_obj::insert (unsigned long sel, Capability cap)
{
    Capability *cte = walk (sel, cap.prm()), old (0);

    if (cte)
        return __atomic_compare_exchange (cte, &old, &cap, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    return false;
}
