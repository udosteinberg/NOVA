/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi_dmar.hpp"
#include "cmdline.hpp"
#include "dmar.hpp"
#include "dpt.hpp"
#include "hip.hpp"
#include "hpet.hpp"
#include "ioapic.hpp"
#include "pci.hpp"
#include "pd.hpp"

void Acpi_dmar::parse() const
{
    Dmar *dmar = new Dmar (static_cast<Paddr>(phys));

    if (flags & 1)
        Pci::claim_all (dmar);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

        switch (s->type) {
            case 1 ... 2:
                Pci::claim_dev (dmar, s->rid());
                break;
            case 3:
                Ioapic::claim_dev (s->rid(), s->id);
                break;
            case 4:
                Hpet::claim_dev (s->rid(), s->id);
                break;
        }
    }
}

void Acpi_rmrr::parse() const
{
    for (uint64 hpa = base & ~OFFS_MASK (0); hpa < limit; hpa += PAGE_SIZE (0))
        Pd::kern.dpt.update (hpa, 0, hpa, Dpt::DPT_R | Dpt::DPT_W);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

        Dmar *dmar = nullptr;

        switch (s->type) {
            case 1:
                dmar = Pci::find_dmar (s->rid());
                break;
        }

        if (dmar)
            dmar->assign (s->rid(), &Pd::kern);
    }
}

void Acpi_table_dmar::parse() const
{
    if (EXPECT_FALSE (Cmdline::nosmmu))
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

    Dmar::enable (flags);

    Hip::hip->set_feature (Hip::FEAT_SMMU);
}
