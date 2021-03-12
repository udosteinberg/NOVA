/*
 * I/O Advanced Programmable Interrupt Controller (IOAPIC)
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

#include "extern.hpp"
#include "ioapic.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ioapic::cache (sizeof (Ioapic), 8);

Ioapic::Ioapic (Paddr p, unsigned i, unsigned g) : List (list), reg_base (mmap | (p & OFFS_MASK)), gsi_base (g), id (i)
{
    mmap += PAGE_SIZE;

#if 0   // FIXME
    Pd::kern.Space_mem::delreg (p & ~OFFS_MASK);
#endif

    Hptp::master_map (reg_base, p & ~OFFS_MASK, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::Cacheability::MEM_UC, Memattr::Shareability::NONE);

    trace (TRACE_INTR, "APIC: %#010lx ID:%#x VER:%#x GSI:%u-%u", p, i, ver(), gsi_base, gsi_base + mre());
}

void Ioapic::init()
{
    for (unsigned i = 0; i <= mre(); i++)
        set_cfg (gsi_base + i);
}
