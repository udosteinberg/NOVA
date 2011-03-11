/*
 * I/O Advanced Programmable Interrupt Controller (I/O APIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "ioapic.h"
#include "initprio.h"
#include "pd.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ioapic::cache (sizeof (Ioapic), 8);

Ioapic *Ioapic::list;

Ioapic::Ioapic (Paddr phys, unsigned gsi, unsigned i) : reg_base ((hwdev_addr -= PAGE_SIZE) | (phys & PAGE_MASK)), gsi_base (gsi), id (i), next (0)
{
    Ioapic **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;

    Pd::kern.Space_mem::delreg (phys & ~PAGE_MASK);
    Pd::kern.Space_mem::insert (reg_base, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, phys & ~PAGE_MASK);

    trace (TRACE_APIC, "APIC:%#lx ID:%#x VER:%#x IRT:%#x PRQ:%u GSI:%u",
           phys, i, version(), irt_max(), prq(), gsi_base);
}
