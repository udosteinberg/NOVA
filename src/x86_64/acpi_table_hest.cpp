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

#include "acpi_table_hest.hpp"
#include "stdio.hpp"

size_t Acpi_table_hest::Source_ghes_1::parse() const
{
    trace (0, "HEST: SRC:%04x REL:%04x Enabled:%u Addr:%#lx/%u Rec:%u Sec:%u Raw:%u Notify:%u", uint16_t { src_id }, uint16_t { rel_id }, uint8_t { enabled }, uint64_t { status.addr }, uint32_t { length }, uint32_t { rec }, uint32_t { sec }, uint32_t { raw }, std::to_underlying (notify.type()));

    return sizeof (*this);
}

void Acpi_table_hest::parse() const
{
    for (auto ptr { reinterpret_cast<uintptr_t>(this + 1) }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ) {

        auto const s { reinterpret_cast<Source const *>(ptr) };

        trace (0, "HEST: Type %u", std::to_underlying (s->type()));

        switch (s->type()) {
            case Source::Type::AER_RP:  ptr += static_cast<Source_aer_rp const *>(s)->parse(); break;
            case Source::Type::AER_EP:  ptr += static_cast<Source_aer_ep const *>(s)->parse(); break;
            case Source::Type::AER_BR:  ptr += static_cast<Source_aer_br const *>(s)->parse(); break;
            case Source::Type::GHES_1:  ptr += static_cast<Source_ghes_1 const *>(s)->parse(); break;
            default: return;
        }
    }
}
