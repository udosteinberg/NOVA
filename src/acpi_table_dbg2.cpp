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

#include "acpi_table_dbg2.hpp"
#include "stdio.hpp"

void Acpi_table_dbg2::parse() const
{
    for (auto ptr { reinterpret_cast<uintptr_t>(this) + info_off }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ) {

        auto const i { reinterpret_cast<Info const *>(ptr) };
        auto const r { reinterpret_cast<Acpi_gas const *>(ptr + i->regs_off) };

        trace (TRACE_FIRM, "DBG2: Console %04x:%04x (%u:%#lx:%u:%u)", uint16_t { i->type }, uint16_t { i->subtype }, std::to_underlying (r->asid), uint64_t { r->addr }, uint8_t { r->bits }, uint8_t { r->accs });

        Console::bind (Debug::Type { uint16_t { i->type } }, Debug::Subtype { uint16_t { i->subtype } }, *r);

        ptr += i->length;
    }
}
