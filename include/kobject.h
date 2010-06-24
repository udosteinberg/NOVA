/*
 * Kernel Object
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

    protected:
        Spinlock    lock;
        uint16      refcount;

        enum
        {
            INVALID,
            PT,
            EC,
            SC,
            PD,
            SM
        };

        explicit Kobject (Pd *pd, mword b, uint8 t) : Mdb (pd, b, 0, 7), objtype (t), refcount (1) {}

    public:
        unsigned type() const { return EXPECT_TRUE (this) ? objtype : 0; }

        ALWAYS_INLINE
        inline bool add_ref()
        {
            for (uint16 r; (r = refcount); )
                if (Atomic::cmp_swap<true>(&refcount, r, static_cast<typeof refcount>(r + 1)))
                    return true;

            return false;
        }

        ALWAYS_INLINE
        inline bool del_ref() { return Atomic::sub (refcount, static_cast<typeof refcount>(1)); }
};
