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

#include "acpi_table_hest.hpp"
#include "stdio.hpp"

bool Acpi_table_hest::parse() const
{
    using list_t = Source;
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { reinterpret_cast<list_t const *>(ptr) };

        trace (TRACE_FIRM | TRACE_PARSE, "HEST: Type:%#x", std::to_underlying (s->type()));

        switch (s->type()) {
            case Source::Type::MCE:     ptr += static_cast<Source_mce    const *>(s)->parse(); break;
            case Source::Type::CMC:     ptr += static_cast<Source_cmc    const *>(s)->parse(); break;
            case Source::Type::NMI:     ptr += static_cast<Source_nmi    const *>(s)->parse(); break;
            case Source::Type::AER_RP:  ptr += static_cast<Source_aer_rp const *>(s)->parse(); break;
            case Source::Type::AER_EP:  ptr += static_cast<Source_aer_ep const *>(s)->parse(); break;
            case Source::Type::AER_BR:  ptr += static_cast<Source_aer_br const *>(s)->parse(); break;
            case Source::Type::GHES_1:  ptr += static_cast<Source_ghes_1 const *>(s)->parse(); break;
            case Source::Type::GHES_2:  ptr += static_cast<Source_ghes_2 const *>(s)->parse(); break;
            case Source::Type::DMC:     ptr += static_cast<Source_dmc    const *>(s)->parse(); break;
            default: return false;
        }
    }

    return end == ptr;
}
