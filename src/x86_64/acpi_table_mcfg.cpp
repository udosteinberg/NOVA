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
#include "string.hpp"

void Acpi_table_mcfg::Segment::parse (char const (&oem)[6], char const (&tbl)[8]) const
{
    auto unusable { false };

    // Ignore segments with broken ECAM
    for (unsigned i { 0 }; i < sizeof (quirk) / sizeof (*quirk); i++)
        if (!strncmp (quirk[i].oem, oem, sizeof (oem)) && !strncmp (quirk[i].tbl, tbl, sizeof (tbl)) && quirk[i].seg & BIT64 (seg))
            unusable = true;

    // We only handle PCI segment group 0 currently
    if (EXPECT_FALSE (unusable || seg)) {
        trace (TRACE_FIRM, "WARN: PCI Segment %#x unusable", seg);
        return;
    }

    trace (TRACE_FIRM, "MCFG: Bus %#04x-%#04x", sbn, ebn);

    Pci::bus_base = 0;
    Pci::cfg_base = static_cast<uint64_t>(phys_base_hi) << 32 | phys_base_lo;
    Pci::cfg_size = (ebn + 1) * 256 * PAGE_SIZE (0);
}

void Acpi_table_mcfg::parse() const
{
    auto const len { table.header.length };
    auto const ptr { reinterpret_cast<uintptr_t>(this) };

    if (EXPECT_FALSE (len < sizeof (*this)))
        return;

    for (auto p { ptr + sizeof (*this) }; p < ptr + len; p += sizeof (Segment))
        reinterpret_cast<Segment const *>(p)->parse (table.oem_id, table.oem_table_id);

    Pci::init();
}
