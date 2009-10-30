/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2008-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "acpi_dmar.h"
#include "cmdline.h"
#include "dmar.h"
#include "stdio.h"

void Acpi_dmar::parse() const
{
    new Dmar (static_cast<Paddr>(base));    // XXX: register DMAR with devices

    if (flags & 1)
        trace (TRACE_DMAR, "DHRD:   All other devices");

    for (Acpi_scope const *acpi_scope = scope;
                           acpi_scope < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length);
                           acpi_scope = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(acpi_scope) + acpi_scope->length)) {
        trace (TRACE_DMAR, "DHRD:   Type:%#x Len:%#x ID:%#x BDF %x:%x:%x",
               acpi_scope->type,
               acpi_scope->length,
               acpi_scope->id,
               acpi_scope->bus,
               acpi_scope->dev,
               acpi_scope->func);
    }
}

void Acpi_rmrr::parse() const
{
    trace (TRACE_DMAR, "RMRR: Segment:%#x %#llx-%#llx", segment, base, limit);

    for (Acpi_scope const *acpi_scope = scope;
                           acpi_scope < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length);
                           acpi_scope = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(acpi_scope) + acpi_scope->length)) {
        trace (TRACE_DMAR, "RMRR:   Type:%#x Len:%#x ID:%#x BDF %x:%x:%x",
               acpi_scope->type,
               acpi_scope->length,
               acpi_scope->id,
               acpi_scope->bus,
               acpi_scope->dev,
               acpi_scope->func);
    }
}

void Acpi_table_dmar::parse() const
{
    if (EXPECT_FALSE (Cmdline::nodmar))
        return;

    trace (TRACE_DMAR, "DMAR: HAW:%u FLAGS:%#x", haw + 1, flags);

    for (Acpi_remap const *acpi_remap = remap;
                           acpi_remap < reinterpret_cast<Acpi_remap *>(reinterpret_cast<mword>(this) + length);
                           acpi_remap = reinterpret_cast<Acpi_remap *>(reinterpret_cast<mword>(acpi_remap) + acpi_remap->length)) {
        switch (acpi_remap->type) {
            case Acpi_remap::DMAR:
                static_cast<Acpi_dmar const *>(acpi_remap)->parse();
                break;
            case Acpi_remap::RMRR:
                static_cast<Acpi_rmrr const *>(acpi_remap)->parse();
                break;
        }
    }
}
