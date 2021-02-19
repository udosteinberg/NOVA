/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi_dmar.hpp"
#include "cmdline.hpp"
#include "hip.hpp"
#include "hpet.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include "pci.hpp"
#include "pd.hpp"
#include "smmu.hpp"

void Acpi_dmar::parse() const
{
    Smmu *smmu = new Smmu (static_cast<Paddr>(phys));

    if (flags & 1)
        Pci::claim_all (smmu);

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

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
    for (uint64 hpa = base & ~OFFS_MASK (0); hpa < limit; hpa += PAGE_SIZE (0))
        Pd::kern.dpt.update (hpa, hpa, 0, Paging::Permissions (Paging::R | Paging::W), Memattr::ram());

    for (Acpi_scope const *s = scope; s < reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(this) + length); s = reinterpret_cast<Acpi_scope *>(reinterpret_cast<mword>(s) + s->length)) {

        Smmu *smmu = nullptr;

        switch (s->type) {
            case 1:
                smmu = Pci::find_smmu (s->rid());
                break;
        }

        if (smmu && !smmu->configured (s->rid()))
            smmu->configure (&Pd::kern, s->rid(), false);
    }
}

void Acpi_table_dmar::parse() const
{
    // Check if firmware opts out of X2APIC support
    if ((flags & BIT_RANGE (1, 0)) == BIT_RANGE (1, 0))
        Lapic::x2apic = false;

    // Disable SMMU due to command line
    if (Cmdline::nosmmu) [[unlikely]]
        return;

    // Enable IR if supported by firmware and SMMU is enabled
    Smmu::ir = flags & BIT (0);

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

    Hip::hip->set_feature (Hip::FEAT_IOMMU);
}
