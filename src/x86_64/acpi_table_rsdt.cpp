/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "hpt.hpp"
#include "util.hpp"

void Acpi_table_rsdt::parse (uint64 addr, size_t size) const
{
    if (!validate (addr))
        return;

    uint64 table[32];
    auto n = min (count (size), sizeof (table) / sizeof (*table));

    // XSDT uses split loads to avoid unaligned accesses
    for (unsigned i = 0; i < n; i++)
        table[i] = size == sizeof (uint64) ? static_cast<uint64>(entry (2 * i + 1)) << 32 | entry (2 * i) : entry (i);

    for (unsigned i = 0; i < n; i++)
        static_cast<Acpi_table *>(Hpt::map (table[i]))->validate (table[i]);
}
