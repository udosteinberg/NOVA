/*
 * Interrupt Descriptor Table (IDT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "descriptor.hpp"
#include "vectors.hpp"

class Idt final : private Descriptor
{
    private:
        uint32 val[sizeof (mword) / 2];

        ALWAYS_INLINE
        inline void set (Type type, unsigned dpl, unsigned selector, mword offset)
        {
            val[0] = static_cast<uint32>(selector << 16 | (offset & 0xffff));
            val[1] = static_cast<uint32>((offset & 0xffff0000) | 1u << 15 | dpl << 13 | type);
            val[2] = static_cast<uint32>(offset >> 32);
        }

    public:
        static Idt idt[NUM_VEC];

        static void build();

        ALWAYS_INLINE
        static inline void load()
        {
            Pseudo_descriptor d { idt, sizeof (idt) };
            asm volatile ("lidt %0" : : "m" (d));
        }
};
