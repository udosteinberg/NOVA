/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

void Acpi_table_xsdt::parse() const
{
    size_t const s { table.header.signature == Signature::u32 ("XSDT") ? sizeof (uint64_t) : sizeof (uint32_t) };

    for (auto ptr { reinterpret_cast<uintptr_t>(this + 1) }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ptr += s)
        Acpi_table::consume (s == sizeof (uint64_t) ? *reinterpret_cast<Unaligned_le<uint64_t> const *>(ptr) : *reinterpret_cast<Unaligned_le<uint32_t> const *>(ptr));
}
