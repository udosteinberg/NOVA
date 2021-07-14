/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "acpi_table_lpit.hpp"
#include "stdio.hpp"

void Acpi_table_lpit::Descriptor_native::parse() const
{
    if (flags & Flags::DISABLED)
        return;

    trace (TRACE_FIRM | TRACE_PARSE, "LPIT: Trigger:%#x/%#lx+%u/%u Counter:%#x/%#lx+%u/%u Residency:%uus Latency:%uus",
           static_cast<uint8_t>(trigger.asid), trigger.addr, trigger.offs, trigger.bits,
           static_cast<uint8_t>(counter.asid), counter.addr, counter.offs, counter.bits,
           min_residency, max_latency);
}

void Acpi_table_lpit::parse() const
{
    auto const ptr { reinterpret_cast<uintptr_t>(this) };

    for (auto p { ptr + sizeof (*this) }; p < ptr + table.header.length; ) {

        auto const d { reinterpret_cast<Descriptor const *>(p) };

        switch (d->type) {
            case Descriptor::NATIVE: static_cast<Descriptor_native const *>(d)->parse(); break;
            default: break;
        }

        p += d->length;
    }
}
