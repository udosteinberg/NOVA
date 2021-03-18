/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_table_mcfg.hpp"
#include "pci.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"

void Acpi_table_mcfg::Allocation::parse() const
{
    // We only handle PCI segment group 0 currently
    if (EXPECT_FALSE (seg)) {
        trace (TRACE_FIRM, "WARN: Platform uses PCI segment group %u", seg);
        return;
    }

    auto virt { MMAP_GLB_PCIE };
    auto phys { static_cast<uint64_t>(phys_base_hi) << 32 | phys_base_lo };
    auto size { static_cast<uint64_t>(ebn + 1) << 20 };

    trace (TRACE_FIRM, "MCFG: %#018lx-%#018lx Bus %#04x-%#04x", phys, phys + size, sbn, ebn);

    for (unsigned o; size; size -= BITN (o), phys += BITN (o), virt += BITN (o))
        Hptp::master_map (virt, phys, (o = static_cast<unsigned>(min (max_order (phys, size), max_order (virt, size)))) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    for (auto bus { sbn }; (bus = Pci::init (bus, ebn)) < ebn; bus++) ;
}

void Acpi_table_mcfg::parse() const
{
    auto const ptr { reinterpret_cast<uintptr_t>(this) };

    for (auto p { ptr + sizeof (*this) }; p < ptr + table.header.length; p += sizeof (Allocation))
        reinterpret_cast<Allocation const *>(p)->parse();
}
