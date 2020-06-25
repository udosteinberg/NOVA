/*
 * Global Descriptor Table (GDT)
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

#include "descriptor.hpp"
#include "selectors.hpp"

class Gdt : public Descriptor
{
    private:
        uint32 val[2];

        ALWAYS_INLINE
        inline void set32 (Type type, Granularity gran, Size size, bool l, unsigned dpl, mword base, mword limit)
        {
            val[0] = static_cast<uint32>(base << 16 | (limit & 0xffff));
            val[1] = static_cast<uint32>((base & 0xff000000) | gran | size | (limit & 0xf0000) | l << 21 | 1u << 15 | dpl << 13 | type | (base >> 16 & 0xff));
        }

        ALWAYS_INLINE
        inline void set64 (Type type, Granularity gran, Size size, bool l, unsigned dpl, mword base, mword limit)
        {
            set32 (type, gran, size, l, dpl, base, limit);
            (this + 1)->val[0] = static_cast<uint32>(base >> 32);
            (this + 1)->val[1] = 0;
        }

    public:
        static Gdt gdt[SEL_MAX >> 3] CPULOCAL;

        static void build();

        ALWAYS_INLINE
        static inline void load()
        {
            asm volatile ("lgdt %0" : : "m" (Pseudo_descriptor (sizeof (gdt) - 1, reinterpret_cast<mword>(gdt))));
        }

        ALWAYS_INLINE
        static inline void unbusy_tss()
        {
            gdt[SEL_TSS_RUN >> 3].val[1] &= ~0x200;
        }
};
