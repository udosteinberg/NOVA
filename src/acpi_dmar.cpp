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
#include "dpt.h"
#include "pci.h"
#include "pd.h"
#include "stdio.h"

void Acpi_dmar::parse() const
{
    Dmar *dmar = new Dmar (static_cast<Paddr>(phys));

    if (flags & 1) {
        trace (TRACE_DMAR, "DHRD:   All other devices");
        Pci::claim_all (dmar);
    }

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

        trace (TRACE_DMAR, "DHRD:   Type:%#x Len:%#x ID:%#x BDF %x:%x:%x",
               s->type, s->length, s->id, s->b, s->d, s->f);

        switch (s->type) {
            case 1:
                Pci::claim_dev (dmar, Bdf (s->b, s->d, s->f));
                break;
            default:
                trace (TRACE_DMAR, "DRHD: Unhandled scope type %#x", s->type);
        }
    }
}

void Acpi_rmrr::parse() const
{
    trace (TRACE_DMAR, "RMRR: Segment:%#x %#llx-%#llx", segment, base, limit);

    for (uint64 hpa = base; hpa < limit; hpa += PAGE_SIZE)
        Pd::kern.dpt()->insert (hpa, 0, Dpt::DPT_R | Dpt::DPT_W, hpa);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

        trace (TRACE_DMAR, "RMRR:   Type:%#x Len:%#x ID:%#x BDF %x:%x:%x",
               s->type, s->length, s->id, s->b, s->d, s->f);

        switch (s->type) {
            case 1:
                Pci::assign_dev (&Pd::kern, Bdf (s->b, s->d, s->f));
                break;
            default:
                trace (TRACE_DMAR, "RMRR: Unhandled scope type %#x", s->type);
        }
    }
}

void Acpi_table_dmar::parse() const
{
    if (EXPECT_FALSE (Cmdline::nodmar))
        return;

    trace (TRACE_DMAR, "DMAR: HAW:%u FLAGS:%#x", haw + 1, flags);

    for (Acpi_remap const *r = remap; r < reinterpret_cast<Acpi_remap *>(reinterpret_cast<mword>(this) + length); r = reinterpret_cast<Acpi_remap *>(reinterpret_cast<mword>(r) + r->length)) {
        switch (r->type) {
            case Acpi_remap::DMAR:
                static_cast<Acpi_dmar const *>(r)->parse();
                break;
            case Acpi_remap::RMRR:
                static_cast<Acpi_rmrr const *>(r)->parse();
                break;
        }
    }

    Dmar::enable_all();
}
