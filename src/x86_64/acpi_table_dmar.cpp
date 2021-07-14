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
#include "ioapic.hpp"
#include "pci.hpp"
#include "pd.hpp"
#include "smmu.hpp"

bool Acpi_table_dmar::Scope::parse (uint16_t const s, pci_t &sbdf) const
{
    uint8_t b { bus };

    using list_t = Path;
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + len };

    while (ptr + sizeof (list_t) <= end) {

        auto const p { std::start_lifetime_as<list_t const> (ptr) };    // Path

        sbdf = Pci::pci (s, b, p->d, p->f);

        // The last pair identifies the target device
        if ((ptr += sizeof (list_t)) == end)
            return true;

#if 0
        // Every other pair identifies a bridge whose secondary bus the next pair resides on
        auto const fun { Pci::Function::lookup (sbdf) };
        if (!fun || (fun->read (Pci::Cfg::Reg8::HDR) & BIT_RANGE (6, 0)) != 1) [[unlikely]]
            return false;

        b = static_cast<uint8_t>(fun->read (Pci::Cfg::Reg32::BUS_NUM) >> 8);
#endif
    }

    return false;
}

bool Acpi_table_dmar::Remapping_drhd::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    auto const smmu { nullptr };
#if 0
    if (!smmu) [[unlikely]]
        panic ("SMMU allocation failed");
#endif

    if (flags & BIT (0))
        Pci::claim_all (smmu);

    using list_t = Scope;
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + len };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };    // Scope
        auto const l { s->len };                                        // Length

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        pci_t sbdf;
        if (!s->parse (seg, sbdf)) [[unlikely]] {
            trace (TRACE_ERROR, "SMMU: %#lx Scope Type %u Failure", uint64_t { phys }, std::to_underlying (s->type()));
            continue;
        }

        trace (TRACE_FIRM | TRACE_PARSE, "SMMU: %#lx Scope Type %u Device %04x:%02x:%02x.%x", uint64_t { phys }, std::to_underlying (s->type()), Pci::seg (sbdf), Pci::bus (sbdf), Pci::dev (sbdf), Pci::fun (sbdf));

        switch (s->type()) {
            case Scope::Type::PCI_EP:
            case Scope::Type::PCI_SH: Pci::claim_dev (smmu, sbdf); break;
            case Scope::Type::IOAPIC: Ioapic::claim_dev (sbdf, s->id); break;
            default: break;
        }
    }

    return end == ptr;
}

bool Acpi_table_dmar::Remapping_rmrr::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    auto const phys_s { aligned_dn (PAGE_SIZE (0), base) };
    auto const phys_e { aligned_up (PAGE_SIZE (0), limit) };

    // Grant DMA read/write access if well-formed memory region
    if (phys_e > phys_s) [[likely]]
        Space_dma::access_ctrl (phys_s, phys_e - phys_s, Paging::Permissions (Paging::W | Paging::R));

    using list_t = Scope;
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + len };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };    // Scope
        auto const l { s->len };                                        // Length

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        pci_t sbdf;
        if (!s->parse (seg, sbdf)) [[unlikely]] {
            trace (TRACE_ERROR, "RMRR: %#010lx-%#010lx Scope Type %u Failure", phys_s, phys_e, std::to_underlying (s->type()));
            continue;
        }

        trace (TRACE_FIRM | TRACE_PARSE, "RMRR: %#010lx-%#010lx Scope Type %u Device %04x:%02x:%02x.%x", phys_s, phys_e, std::to_underlying (s->type()), Pci::seg (sbdf), Pci::bus (sbdf), Pci::dev (sbdf), Pci::fun (sbdf));

        Smmu *smmu { nullptr }; uintptr_t x;

        switch (s->type()) {
            case Scope::Type::PCI_EP: smmu = Pci::find_smmu (sbdf); break;
            default: break;
        }

        // Assign firmware-driven DMA device to the NOVA DMA space
        if (smmu && !smmu->nova_assigned (sbdf) && smmu->assign_dev (sbdf, nullptr, &Pd::kern, x) != Status::SUCCESS) [[unlikely]]
            break;
    }

    return end == ptr;
}

bool Acpi_table_dmar::parse() const
{
    // Check if firmware opts out of x2APIC support for the platform
    if ((flags & BIT_RANGE (1, 0)) == BIT_RANGE (1, 0)) [[unlikely]]
        Lapic::x2apic = false;

    // Check if firmware opts out of interrupt remapping
    Smmu::noir |= !(flags & BIT (0));

    using list_t = Remapping;
    bool       ret { true };
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };
        auto const l { s->len };

        trace (TRACE_FIRM | TRACE_PARSE, "DMAR: Type:%#x Len:%u", std::to_underlying (s->type()), unsigned { l });

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        switch (s->type()) {
            case Remapping::Type::DRHD: ret &= std::start_lifetime_as<Remapping_drhd const> (s)->parse(); break;
            case Remapping::Type::RMRR: ret &= std::start_lifetime_as<Remapping_rmrr const> (s)->parse(); break;
            default: break;
        }
    }

    return ret && end == ptr;
}
