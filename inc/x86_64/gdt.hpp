/*
 * Global Descriptor Table (GDT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

class Gdt final
{
    private:
        Descriptor_gdt_seg null;        // 0x0
        Descriptor_gdt_seg kern_code;   // 0x8
        Descriptor_gdt_seg kern_data;   // 0x10
        Descriptor_gdt_seg user_data;   // 0x18
        Descriptor_gdt_seg user_code;   // 0x20
        Descriptor_gdt_seg unused;      // 0x28
        Descriptor_gdt_sys tss_run;     // 0x30

    public:
        static Gdt gdt CPULOCAL;

        static void build();

        static void load()
        {
            Pseudo_descriptor const d { &gdt, sizeof (gdt) };
            asm volatile ("lgdt %0" : : "m" (d));
        }

        static void unbusy_tss()
        {
            gdt.tss_run.val[1] &= ~BIT (9);
        }
};

static_assert (__is_standard_layout (Gdt) && sizeof (Gdt) == SEL_MAX);
