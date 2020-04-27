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

#include "acpi.hpp"
#include "acpi_table.hpp"
#include "stdio.hpp"

bool Acpi_table::validate (uint64_t phys, bool override) const
{
    auto const v { valid() };

    trace (TRACE_FIRM, "%.4s: %#010lx REV:%2d TBL:%8.8s OEM:%6.6s LEN:%7u (%#04x) %s",
           reinterpret_cast<char const *>(&header.signature), phys, revision,
           oem_table_id, oem_id, header.length, checksum, v ? "ok" : "bad");

    // A valid table can replace an existing table only if override is true
    if (EXPECT_TRUE (v))
        for (unsigned i { 0 }; i < sizeof (Acpi::tables) / sizeof (*Acpi::tables); i++)
            if (Acpi::tables[i].sig == header.signature && (override || !Acpi::tables[i].var))
                Acpi::tables[i].var = phys;

    return v;
}
