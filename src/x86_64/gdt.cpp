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

#include "gdt.hpp"
#include "memory.hpp"
#include "tss.hpp"

ALIGNED(8) Gdt Gdt::gdt;

void Gdt::build()
{
    gdt.kern_code = Descriptor_gdt_seg { Descriptor_gdt_seg::Type::CODE_XRA, 0 };
    gdt.kern_data = Descriptor_gdt_seg { Descriptor_gdt_seg::Type::DATA_RWA, 0 };
    gdt.user_data = Descriptor_gdt_seg { Descriptor_gdt_seg::Type::DATA_RWA, 3 };
    gdt.user_code = Descriptor_gdt_seg { Descriptor_gdt_seg::Type::CODE_XRA, 3 };

    gdt.tss_run = Descriptor_gdt_sys { Descriptor_gdt_sys::Type::SYS_TSS, reinterpret_cast<uint64_t>(&Tss::run), static_cast<uint32_t>(MMAP_SPC_PIO_E - reinterpret_cast<uintptr_t>(&Tss::run)) };
}
