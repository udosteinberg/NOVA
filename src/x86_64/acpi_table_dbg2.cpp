/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

void Acpi_table_dbg2::parse() const
{
    for (auto d { reinterpret_cast<uint8_t const *>(this) + info_off }; d < reinterpret_cast<uint8_t const *>(this) + table.header.length; ) {

        auto const i { reinterpret_cast<Info const *>(d) };
        auto const r { reinterpret_cast<Acpi_gas const *>(d + i->regs_off) };

        if (i->type != Debug::Type::SERIAL)
            continue;

        trace (TRACE_FIRM, "DBG2: Console %#x at %u:%#lx:%u", std::to_underlying (i->subtype), std::to_underlying (r->asid), r->addr, r->bits);

        Console::bind (r, i->subtype);

        d += i->length;
    }
}
