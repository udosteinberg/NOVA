/*
 * Global Descriptor Table (GDT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

ALIGNED(8) Gdt Gdt::gdt[SEL_MAX >> 3];

void Gdt::build()
{
    Size s = BIT_16;
    bool l = true;

    gdt[SEL_KERN_CODE >> 3].set32 (CODE_XRA, PAGES, s, l, 0, 0, ~0ul);
    gdt[SEL_KERN_DATA >> 3].set32 (DATA_RWA, PAGES, s, l, 0, 0, ~0ul);

    gdt[SEL_USER_DATA >> 3].set32 (DATA_RWA, PAGES, s, l, 3, 0, ~0ul);
    gdt[SEL_USER_CODE >> 3].set32 (CODE_XRA, PAGES, s, l, 3, 0, ~0ul);

    gdt[SEL_TSS_RUN >> 3].set64 (SYS_TSS, BYTES, BIT_16, false, 0, reinterpret_cast<mword>(&Tss::run), MMAP_SPC_PIO_E - reinterpret_cast<mword>(&Tss::run));
    gdt[SEL_TSS_DBF >> 3].set64 (SYS_TSS, BYTES, BIT_16, false, 0, reinterpret_cast<mword>(&Tss::dbf), sizeof (Tss) - 1);
}
