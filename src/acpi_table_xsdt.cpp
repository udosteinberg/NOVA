/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "ptab_hpt.hpp"

void Acpi_table_xsdt::parse (size_t size) const
{
    auto const len { table.header.length };
    auto const ptr { reinterpret_cast<uintptr_t>(this) };

    if (EXPECT_FALSE (len < sizeof (*this)))
        return;

    for (auto p { ptr + sizeof (*this) }; p < ptr + len; p += size) {

        auto const e { reinterpret_cast<uint32_t const *>(p) };

        // XSDT uses split loads to avoid unaligned accesses
        auto const phys { (size == sizeof (uint64_t) ? static_cast<uint64_t>(*(e + 1)) << 32 : 0) | *e};

        if (EXPECT_TRUE (phys))
            static_cast<Acpi_table *>(Hptp::map (MMAP_GLB_MAP1, phys))->validate (phys);
    }
}
