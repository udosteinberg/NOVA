/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_table_xsdt.hpp"

bool Acpi_table_xsdt::parse() const
{
    auto const l { table.header.signature == Signature::u32 ("XSDT") ? sizeof (uint64_t) : sizeof (uint32_t) };

    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    for (; ptr + l <= end; ptr += l)
        Acpi_table::consume (l == sizeof (uint64_t) ? *std::start_lifetime_as<Unaligned_le<uint64_t> const> (ptr) : *std::start_lifetime_as<Unaligned_le<uint32_t> const> (ptr));

    return end == ptr;
}
