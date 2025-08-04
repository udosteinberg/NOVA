/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "dc.hpp"
#include "hpet.hpp"
#include "ioapic.hpp"
#include "pci.hpp"
#include "smmu_itl.hpp"
#include "space_dma.hpp"

bool Acpi_table_dmar::Remapping_drhd::parse() const
{
    auto const smmu { Smmu_itl::create (phys, Pci::pci (seg, 0)) };
    if (!smmu) [[unlikely]]
        panic ("SMMU allocation failed");

    if (flags & BIT (0))
        Pci::Function::claim_all (smmu);

    using list_t = Scope;
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + len };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { reinterpret_cast<list_t const *>(ptr) };     // Scope
        auto const l { s->len };                                    // Length
        auto const t { Pci::pci (seg, s->b, s->d, s->f) };          // Topology

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        trace (TRACE_FIRM | TRACE_PARSE, "SMMU: %#lx Scope Type %u Device %04x:%02x:%02x.%x", uint64_t { phys }, std::to_underlying (s->type()), Pci::seg (t), Pci::bus (t), Pci::dev (t), Pci::fun (t));

        switch (s->type()) {
            case Scope::Type::PCI_EP:
            case Scope::Type::PCI_SH: Pci::Function::claim_dev (smmu, t); break;
            case Scope::Type::IOAPIC: Ioapic::claim_dev (t, s->id); break;
            case Scope::Type::HPET: Hpet::claim_dev (t, s->id); break;
            default: break;
        }
    }

    return end == ptr;
}

bool Acpi_table_dmar::Remapping_rmrr::parse() const
{
    auto const phys_s { aligned_dn (PAGE_SIZE (0), base) };
    auto const phys_e { aligned_up (PAGE_SIZE (0), limit) };

    // Grant DMA read/write access if well-formed memory region
    if (phys_e > phys_s) [[likely]]
        Space_dma::access_ctrl (phys_s, phys_e - phys_s, Paging::Permissions (Paging::W | Paging::R));

    using list_t = Scope;
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + len };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { reinterpret_cast<list_t const *>(ptr) };     // Scope
        auto const l { s->len };                                    // Length
        auto const t { Pci::pci (seg, s->b, s->d, s->f) };          // Topology

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        trace (TRACE_FIRM | TRACE_PARSE, "RMRR: %#010lx-%#010lx Scope Type %u Device %04x:%02x:%02x.%x", phys_s, phys_e, std::to_underlying (s->type()), Pci::seg (t), Pci::bus (t), Pci::dev (t), Pci::fun (t));

        Smmu *smmu { nullptr };

        switch (s->type()) {
            case Scope::Type::PCI_EP: smmu = Pci::Function::find_smmu (t); break;
            default: break;
        }

        if (smmu/* && !smmu->configured (t)*/) {
            Refptr<Pd> ref;
            Dc const dc { ref, t };
            smmu->assign_dev (&dc, &Space_dma::nova, false);
        }
    }

    return end == ptr;
}

bool Acpi_table_dmar::parse() const
{
    // Check if firmware opts out of X2APIC support
    if ((flags & BIT_RANGE (1, 0)) == BIT_RANGE (1, 0)) [[unlikely]]
        Lapic::x2apic = false;

    // Enable IR if supported by firmware and SMMU is enabled
    Smmu_itl::ir = flags & BIT (0);

    using list_t = Remapping;
    bool       ret { true };
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { reinterpret_cast<list_t const *>(ptr) };
        auto const l { s->len };

        trace (TRACE_FIRM | TRACE_PARSE, "DMAR: Type:%#x Len:%u", std::to_underlying (s->type()), unsigned { l });

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        switch (s->type()) {

            default:
                break;

            case Remapping::Type::DRHD:
                ret &= static_cast<Remapping_drhd const *>(s)->parse();
                break;

            case Remapping::Type::RMRR:
                ret &= static_cast<Remapping_rmrr const *>(s)->parse();
                break;
        }
    }

    return ret && end == ptr;
}
