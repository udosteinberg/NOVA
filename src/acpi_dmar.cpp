/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2008-2010, Udo Steinberg <udo@hypervisor.org>
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

    if (flags & 1)
        Pci::claim_all (dmar);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

        switch (s->type) {
            case 1:
                Pci::claim_dev (dmar, s->b << 8 | s->d << 3 | s->f);
                break;
        }
    }
}

void Acpi_rmrr::parse() const
{
    for (uint64 hpa = base; hpa < limit; hpa += PAGE_SIZE)
        Pd::kern.dpt->insert (hpa, 0, Dpt::DPT_R | Dpt::DPT_W, hpa);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

        switch (s->type) {
            case 1:
                unsigned rid = s->b << 8 | s->d << 3 | s->f;
                Dmar *dmar = Pci::find_dmar (rid);
                if (dmar)
                    dmar->assign (rid, &Pd::kern);
                break;
        }
    }
}

void Acpi_table_dmar::parse() const
{
    if (!Cmdline::dmar)
        return;

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
