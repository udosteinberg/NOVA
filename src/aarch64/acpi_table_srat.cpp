/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_table_srat.hpp"
#include "stdio.hpp"

void Acpi_table_srat::Affinity_memory::parse() const
{
    // Skip disabled entries
    if (!(flags & BIT (0))) [[unlikely]]
        return;

    trace (TRACE_FIRM, "SRAT: %#018lx-%018lx Dom %u", uint64_t { base }, base + size, uint32_t { pxd });
}

void Acpi_table_srat::parse() const
{
    for (auto ptr { reinterpret_cast<uintptr_t>(this + 1) }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ) {

        auto const a { reinterpret_cast<Affinity const *>(ptr) };

        switch (a->type()) {
            case Affinity::Type::MEMORY: static_cast<Affinity_memory const *>(a)->parse(); break;
            default: break;
        }

        ptr += a->length;
    }
}
