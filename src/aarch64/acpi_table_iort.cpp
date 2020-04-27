/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "stdio.hpp"

void Acpi_table_iort::Node_smmu_v2::parse() const
{
}

void Acpi_table_iort::Node_smmu_v3::parse() const
{
}

void Acpi_table_iort::parse() const
{
    for (auto ptr { reinterpret_cast<uintptr_t>(this) + node_ofs }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ) {

        auto const n { reinterpret_cast<Node const *>(ptr) };

        trace (TRACE_FIRM | TRACE_PARSE, "IORT: Node Type %u", std::to_underlying (n->type()));

        switch (n->type()) {
            case Node::Type::SMMU_V2: static_cast<Node_smmu_v2 const *>(n)->parse(); break;
            case Node::Type::SMMU_V3: static_cast<Node_smmu_v3 const *>(n)->parse(); break;
            default: break;
        }

        ptr += n->len;
    }
}
