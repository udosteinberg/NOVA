/*
 * Kernel Object
 *
 * Copyright (C) 2009, Udo Steinberg <udo@hypervisor.org>
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
#include "types.h"

class Kobject
{
    private:
        uint8   objtype;
        uint8   misc;
        uint16  refcount;

    protected:
        enum
        {
            NUL,
            PT,
            EC,
            SC,
            PD,
            SM
        };

        Kobject (uint8 t, uint16 r) : objtype (t), refcount (r) {}

    public:
        unsigned type() const { return EXPECT_TRUE (this) ? objtype : 0; }

        ALWAYS_INLINE
        inline void add_ref() { Atomic::add (refcount, 1); }

        ALWAYS_INLINE
        inline bool del_ref() { return Atomic::sub (refcount, 1); }
};
