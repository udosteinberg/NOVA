/*
 * Kernel Object
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "mdb.hpp"
#include "refptr.hpp"

class Kobject : public Mdb
{
    private:
        uint8 objtype;

        static void free (Rcu_elem *) {}

    protected:
        Spinlock lock;

        enum Type
        {
            PD,
            EC,
            SC,
            PT,
            SM,
            INVALID,
        };

        explicit Kobject (Type t, Space *s, mword b = 0, mword a = 0) : Mdb (s, reinterpret_cast<mword>(this), b, a, free), objtype (t) {}

    public:
        ALWAYS_INLINE
        inline Type type() const
        {
            return EXPECT_TRUE (this) ? Type (objtype) : INVALID;
        }
};
