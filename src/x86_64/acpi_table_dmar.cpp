/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "acpi_table_dmar.hpp"
#include "cmdline.hpp"
#include "hip.hpp"
#include "hpet.hpp"
#include "ioapic.hpp"
#include "pci.hpp"
#include "pd_kern.hpp"
#include "smmu.hpp"

void Acpi_dmar::parse() const
{
    Smmu *smmu = new Smmu (static_cast<Paddr>(phys));

    if (flags & 1)
        Pci::claim_all (smmu);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<uintptr_t>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<uintptr_t>(s) + s->length)) {

        switch (s->type) {
            case 1 ... 2:
                Pci::claim_dev (smmu, s->rid());
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
    for (uint64 hpa = base & ~PAGE_MASK; hpa < limit; hpa += PAGE_SIZE)
        Pd_kern::nova().dpt.update (hpa, hpa, 0, Paging::Permissions (Paging::R | Paging::W), Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<uintptr_t>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<uintptr_t>(s) + s->length)) {

        Smmu *smmu = nullptr;

        switch (s->type) {
            case 1:
                smmu = Pci::find_smmu (s->rid());
                break;
        }

        if (smmu)
            smmu->configure (&Pd_kern::nova(), Space::Index::DMA_HST, s->rid());
    }
}

void Acpi_table_dmar::parse() const
{
    if (!Cmdline::iommu)
        return;

    for (Acpi_remap const *r = remap; r < reinterpret_cast<Acpi_remap *>(reinterpret_cast<uintptr_t>(this) + length); r = reinterpret_cast<Acpi_remap *>(reinterpret_cast<uintptr_t>(r) + r->length)) {
        switch (r->type) {
            case Acpi_remap::DMAR:
                static_cast<Acpi_dmar const *>(r)->parse();
                break;
            case Acpi_remap::RMRR:
                static_cast<Acpi_rmrr const *>(r)->parse();
                break;
        }
    }

    Smmu::enable (flags);

    Hip::set_feature (Hip_arch::Feature::IOMMU);
}
