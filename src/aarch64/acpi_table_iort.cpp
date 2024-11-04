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

#include "acpi_table_iort.hpp"
#include "smmu_v3.hpp"

bool Acpi_table_iort::Node_smmu_v2::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    return true;
}

bool Acpi_table_iort::Node_smmu_v3::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    auto const smmu { Smmu_v3::create (base, { intid_evt, intid_pri, intid_glb, intid_cmd }) };
    if (!smmu) [[unlikely]]
        panic ("SMMU allocation failed");

    return true;
}

bool Acpi_table_iort::parse() const
{
    using list_t = Node;
    bool       ret { true };
    auto       ptr { reinterpret_cast<uintptr_t>(this) + min (uint32_t { table.header.length }, uint32_t { node_ofs }) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };
        auto const l { s->len };

        trace (TRACE_FIRM | TRACE_PARSE, "IORT: Type:%#x Len:%u", std::to_underlying (s->type()), unsigned { l });

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        switch (s->type()) {
            case Node::Type::SMMU_V2: ret &= std::start_lifetime_as<Node_smmu_v2 const> (s)->parse(); break;
            case Node::Type::SMMU_V3: ret &= std::start_lifetime_as<Node_smmu_v3 const> (s)->parse(); break;
            default: break;
        }
    }

    return ret && end == ptr;
}
