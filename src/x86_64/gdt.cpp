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

#include "gdt.hpp"
#include "memory.hpp"
#include "tss.hpp"

ALIGNED(8) Gdt Gdt::gdt[SEL_MAX >> 3];

void Gdt::build()
{
#ifdef __i386__
    Size s = BIT_32;
    bool l = false;
#else
    Size s = BIT_16;
    bool l = true;
#endif
    gdt[SEL_KERN_CODE   >> 3].set32 (CODE_XRA, PAGES, s, l, 0, 0, ~0ul);
    gdt[SEL_KERN_DATA   >> 3].set32 (DATA_RWA, PAGES, s, l, 0, 0, ~0ul);

    gdt[SEL_USER_CODE   >> 3].set32 (CODE_XRA, PAGES, s, l, 3, 0, ~0ul);
    gdt[SEL_USER_DATA   >> 3].set32 (DATA_RWA, PAGES, s, l, 3, 0, ~0ul);
    gdt[SEL_USER_CODE_L >> 3].set32 (CODE_XRA, PAGES, s, l, 3, 0, ~0ul);

#ifdef __i386__
    gdt[SEL_TSS_RUN >> 3].set32 (SYS_TSS, BYTES, BIT_16, false, 0, reinterpret_cast<mword>(&Tss::run), SPC_LOCAL_IOP_E - reinterpret_cast<mword>(&Tss::run));
    gdt[SEL_TSS_DBF >> 3].set32 (SYS_TSS, BYTES, BIT_16, false, 0, reinterpret_cast<mword>(&Tss::dbf), sizeof (Tss) - 1);
#else
    gdt[SEL_TSS_RUN >> 3].set64 (SYS_TSS, BYTES, BIT_16, false, 0, reinterpret_cast<mword>(&Tss::run), SPC_LOCAL_IOP_E - reinterpret_cast<mword>(&Tss::run));
    gdt[SEL_TSS_DBF >> 3].set64 (SYS_TSS, BYTES, BIT_16, false, 0, reinterpret_cast<mword>(&Tss::dbf), sizeof (Tss) - 1);
#endif
}
