/*
 * I/O Advanced Programmable Interrupt Controller (IOAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Ioapic::cache { sizeof (Ioapic), alignof (Ioapic) };

void Ioapic::init() const
{
    trace (TRACE_INTR, "APIC: %#010lx %04x:%02x:%02x.%x VER:%#x GSI:%#04x-%#04x", phys, Pci::seg (sbdf), Pci::bus (sbdf), Pci::dev (sbdf), Pci::fun (sbdf), ver(), gsi_base, gsi_last);

    // Ensure ID[31:24] is consistent with firmware tables
    write (Ind32::ID, id << 24);

    // Mask all entries
    for (auto pin { gsi_last - gsi_base + 1 }; pin--; rte_mask (pin)) {}
}
