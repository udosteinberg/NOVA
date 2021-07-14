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

#include "acpi_table_dbg2.hpp"
#include "stdio.hpp"

bool Acpi_table_dbg2::parse() const
{
    using list_t = Info;
    auto       ptr { reinterpret_cast<uintptr_t>(this) + min (uint32_t { table.header.length }, uint32_t { info_off }) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { reinterpret_cast<list_t const *>(ptr) };
        auto const l { s->len };
        auto const r { reinterpret_cast<Acpi_gas const *>(ptr + s->regs_off) };

        trace (TRACE_FIRM | TRACE_PARSE, "DBG2: Type:%#x Len:%u", uint16_t { s->type }, unsigned { l });

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end || reinterpret_cast<uintptr_t>(r + 1) > end) [[unlikely]]
            break;

        trace (TRACE_FIRM, "DBG2: Console %04x:%04x (%u:%#lx:%u:%u)", uint16_t { s->type }, uint16_t { s->subtype }, std::to_underlying (r->asid), uint64_t { r->addr }, uint8_t { r->bits }, uint8_t { r->accs });

        Console::bind (Debug::Type { uint16_t { s->type } }, Debug::Subtype { uint16_t { s->subtype } }, *r);
    }

    return end == ptr;
}
