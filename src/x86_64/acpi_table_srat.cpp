/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_table_srat.hpp"
#include "stdio.hpp"

bool Acpi_table_srat::Affinity_lapic::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    return true;
}

bool Acpi_table_srat::Affinity_x2apic::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    return true;
}

bool Acpi_table_srat::Affinity_memory::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    // Only enabled entries
    if (flags & BIT (0)) [[likely]]
        trace (TRACE_FIRM, "SRAT: %#018lx-%018lx Dom %u", uint64_t { base }, base + size, uint32_t { pxd });

    return true;
}

bool Acpi_table_srat::parse() const
{
    using list_t = Affinity;
    bool       ret { true };
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };
        auto const l { s->len };

        trace (TRACE_FIRM | TRACE_PARSE, "SRAT: Type:%#x Len:%u", std::to_underlying (s->type()), unsigned { l });

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        switch (s->type()) {
            case Affinity::Type::LAPIC:  ret &= std::start_lifetime_as<Affinity_lapic  const> (s)->parse(); break;
            case Affinity::Type::MEMORY: ret &= std::start_lifetime_as<Affinity_memory const> (s)->parse(); break;
            case Affinity::Type::X2APIC: ret &= std::start_lifetime_as<Affinity_x2apic const> (s)->parse(); break;
            default: break;
        }
    }

    return ret && end == ptr;
}
