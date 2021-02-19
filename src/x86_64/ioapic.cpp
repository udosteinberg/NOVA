/*
 * I/O Advanced Programmable Interrupt Controller (IOAPIC)
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

#include "ioapic.hpp"
#include "pd.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ioapic::cache (sizeof (Ioapic), 8);

Ioapic *Ioapic::list;

Ioapic::Ioapic (Paddr p, unsigned i, unsigned g) : List<Ioapic> (list), reg_base ((hwdev_addr -= PAGE_SIZE) | (p & PAGE_MASK)), gsi_base (g), id (i), rid (0)
{
#if 0   // FIXME
    Pd::kern.Space_mem::delreg (p & ~PAGE_MASK);
#endif

    Pd::kern.Space_mem::insert (reg_base, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, p & ~PAGE_MASK);

    trace (TRACE_INTR, "APIC:%#lx ID:%#x VER:%#x IRT:%#x PRQ:%u GSI:%u",
           p, i, version(), irt_max(), prq(), gsi_base);
}
