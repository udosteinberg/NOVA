/*
 * Kernel Object
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "atomic.h"
#include "compiler.h"
#include "mdb.h"
#include "spinlock.h"

class Kobject : public Mdb
{
    private:
        uint8       objtype;
        uint8       refcount;

    protected:
        Spinlock    lock;

        enum Type
        {
            PD,
            EC,
            SC,
            PT,
            SM,
            INVALID,
        };

        explicit Kobject (Type t, Pd *pd, mword b, mword a = perm) : Mdb (pd, reinterpret_cast<mword>(this), b, 0, a), objtype (t), refcount (1) {}

    public:
        static mword const perm = 0x1f;

        Type type() const { return EXPECT_TRUE (this) ? Type (objtype) : INVALID; }

        ALWAYS_INLINE
        inline bool add_ref()
        {
            for (uint8 r; (r = refcount); )
                if (Atomic::cmp_swap (refcount, r, static_cast<typeof refcount>(r + 1)))
                    return true;

            return false;
        }

        ALWAYS_INLINE
        inline bool del_ref() { return Atomic::sub (refcount, static_cast<typeof refcount>(1)) == 0; }
};
