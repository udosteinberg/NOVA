/*
 * Global Descriptor Table (GDT)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "compiler.h"
#include "descriptor.h"
#include "selectors.h"

class Gdt : public Descriptor
{
    private:
        uint32 val[2];

        ALWAYS_INLINE
        inline void set (Type type, Granularity gran, Size size, unsigned dpl, mword base, mword limit)
        {
            val[0] = static_cast<uint32>(base << 16 | (limit & 0xffff));
            val[1] = static_cast<uint32>((base & 0xff000000) | gran | size | (limit & 0xf0000) | 1u << 15 | dpl << 13 | type | (base >> 16 & 0xff));
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
