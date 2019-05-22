/*
 * Capability
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "atomic.hpp"
#include "compiler.hpp"
#include "kobject.hpp"

class Capability
{
    private:
        mword val { 0 };

        static mword const perm = 0x1f;

    public:
        explicit Capability (mword v) : val (v) {}

        Capability (Kobject *o, mword a) : val (a ? reinterpret_cast<mword>(o) | (a & perm) : 0) {}

        ALWAYS_INLINE
        inline Kobject *obj() const { return reinterpret_cast<Kobject *>(val & ~perm); }

        ALWAYS_INLINE
        inline unsigned prm() const { return val & perm; }

        ALWAYS_INLINE
        bool validate (Kobject::Type t, unsigned p) const
        {
            return obj() && obj()->type == t && prm() & 1UL << p;
        }

        static Capability exchange (Capability *ptr, Capability cap)
        {
            return Capability (Atomic::exchange (ptr->val, cap.val));
        }

        static bool compare_exchange (Capability *ptr, Capability old, Capability cap)
        {
            return Atomic::cmp_swap (ptr->val, old.val, cap.val);
        }
};
