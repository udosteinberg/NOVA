/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "dmar.hpp"
#include "hip.hpp"
#include "hpet.hpp"
#include "ioapic.hpp"
#include "pci.hpp"
#include "pd.hpp"
#include "stdio.hpp"

void Acpi_table_dmar::Remapping_drhd::parse() const
{
    auto dmar = new Dmar (phys);

    if (flags & Flags::INCLUDE_PCI_ALL)
        Pci::claim_all (dmar);

    auto addr = reinterpret_cast<uintptr_t>(this);

    for (auto a = addr + sizeof (*this); a < addr + length; ) {

        auto s = reinterpret_cast<Scope const *>(a);

        trace (TRACE_FIRM | TRACE_PARSE, "SMMU: %#llx Scope Type %u Device %02x:%02x.%x", phys, s->type, s->b, s->d, s->f);

        switch (s->type) {
            case Scope::PCI_EP:
            case Scope::PCI_SH: Pci::claim_dev (dmar, s->dev()); break;
            case Scope::IOAPIC: Ioapic::claim_dev (s->dev(), s->id); break;
            case Scope::HPET: Hpet::claim_dev (s->dev(), s->id); break;
            default: break;
        }

        a += s->length;
    }
}

void Acpi_table_dmar::Remapping_rmrr::parse() const
{
    for (uint64 hpa = base & ~OFFS_MASK; hpa < limit; hpa += PAGE_SIZE)
        Pd::kern.Space_dma::update (hpa, hpa, 0, Paging::Permissions (Paging::W | Paging::R), Memattr::ram());

    auto addr = reinterpret_cast<uintptr_t>(this);

    for (auto a = addr + sizeof (*this); a < addr + length; ) {

        auto s = reinterpret_cast<Scope const *>(a);

        trace (TRACE_FIRM | TRACE_PARSE, "RMRR: %#010llx-%#010llx Scope Type %u Device %02x:%02x.%x", base, limit, s->type, s->b, s->d, s->f);

        Dmar *dmar = nullptr;

        switch (s->type) {
            case Scope::PCI_EP: dmar = Pci::find_dmar (s->dev()); break;
            default: break;
        }

        if (dmar)
            dmar->assign (s->dev(), &Pd::kern);

        a += s->length;
    }
}

void Acpi_table_dmar::parse() const
{
    if (EXPECT_FALSE (Cmdline::nosmmu))
        return;

    auto addr = reinterpret_cast<uintptr_t>(this);

    for (auto a = addr + sizeof (*this); a < addr + table.header.length; ) {

        auto r = reinterpret_cast<Remapping const *>(a);

        switch (r->type) {
            case Remapping::DRHD: static_cast<Remapping_drhd const *>(r)->parse(); break;
            case Remapping::RMRR: static_cast<Remapping_rmrr const *>(r)->parse(); break;
            default: break;
        }

        a += r->length;
    }

    Dmar::enable (flags);

    Hip::hip->set_feature (Hip::FEAT_IOMMU);
}
