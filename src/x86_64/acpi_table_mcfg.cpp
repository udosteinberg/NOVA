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
#include "stdio.hpp"

void Acpi_table_mcfg::Allocation::parse() const
{
    // We only handle PCI segment group 0 currently
    if (seg)
        return;

    Pci::bus_base = bus_s;
    Pci::cfg_base = static_cast<uint64_t>(phys_base_hi) << 32 | phys_base_lo;
    Pci::cfg_size = ((bus_e - bus_s + 1) << 8) * PAGE_SIZE;

    trace (TRACE_FIRM | TRACE_PARSE, "MCFG: %#lx Bus %u-%u", Pci::cfg_base, bus_s, bus_e);
}

void Acpi_table_mcfg::parse() const
{
    auto const addr { reinterpret_cast<uintptr_t>(this) };

    for (auto a { addr + sizeof (*this) }; a < addr + table.header.length; a += sizeof (Allocation))
        reinterpret_cast<Allocation const *>(a)->parse();

    Pci::init();
}
