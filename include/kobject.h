/*
 * Kernel Object
 *
 * Copyright (C) 2009-2010, Udo Steinberg <udo@hypervisor.org>
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
#include "spinlock.h"
#include "vma.h"

class Kobject : public Vma
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

        explicit Kobject (Pd *own, mword sel, uint8 t, uint16 r) : Vma (own, sel, 0, 0, 0), objtype (t), refcount (r) {}

    public:
        unsigned type() const { return EXPECT_TRUE (this) ? objtype : 0; }

        ALWAYS_INLINE
        inline void add_ref() { Atomic::add (refcount, 1); }

        ALWAYS_INLINE
        inline bool del_ref() { return Atomic::sub (refcount, 1); }
};
