/*
 * System Memory Management Unit (Intel IOMMU)
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

#include "bits.hpp"
#include "dc.hpp"
#include "ioapic.hpp"
#include "smmu_itl.hpp"
#include "space_dma.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu_itl::cache { sizeof (Smmu_itl), alignof (Smmu_itl) };

uint32_t Smmu_itl::sequence {}, Smmu_itl::doorbell {};

Smmu_itl::Smmu_itl (uint64_t p, size_t s, pci_t t, Devtable *d, Inttable *i) : Smmu { p, s, t }, cap { read (Reg64::CAP) }, ecap { read (Reg64::ECAP) }, dtbl { d }, itbl { i }, invq { ord_inv }
{
    // Set DPT maximum leaf level
    Dptp_itl::set_mll (mll());

    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt_itl::noncoherent |= !feature (Ecap::PWC);

    // If the SMMU does not support interrupt remapping, then disable IR globally
    noir |= !feature (Ecap::IR);

    // Operate in x2APIC mode only if the LAPICs and the SMMU support it
    x2apic &= Lapic::x2apic && feature (Ecap::EIM);

    // Reduce the number of domain IDs to what this SMMU supports
    Sdid::allocator.reduce (BIT (2 * nid() + 4));
}

Smmu_itl *Smmu_itl::create (uint64_t phys, size_t size, pci_t sbdf)
{
    // Set SMMU list type to ITL. Success makes the list downcast safe
    if (!set_type (Type::ITL)) [[unlikely]]
        return nullptr;

    // Look for sibling SMMU in the same PCI segment group
    auto const sibling { static_cast<Smmu_itl *>(lookup_seg (Pci::seg (sbdf))) };

    // Share per-segment data structures with siblings
    auto const dtbl { sibling ? sibling->dtbl : new Devtable };
    auto const itbl { sibling ? sibling->itbl : new Inttable };

    // Allocate SMMU
    auto const smmu { new Smmu_itl { phys, size, sbdf, dtbl, itbl } };

    // Add to SMMU list only if all structure allocations succeeded
    if (smmu && smmu->dtbl && smmu->itbl && smmu->invq.ptr) [[likely]] {
        smmu->insert (list);
        return smmu;
    }

    return nullptr;
}

bool Smmu_itl::init()
{
    auto const addr { Lapic::msi_addr (0) };

    // Check if hardware mandates IR but it is disabled
    if (feature (Ecap::IRREQ) && noir) [[unlikely]]
        return false;

    // Check if hardware mandates x2APIC but the operational mode is xAPIC
    if (feature (Ecap::EIMER) && !x2apic) [[unlikely]]
        return false;

    // Check if all registers at FRO are covered by the MMIO region
    if (mmio_size < fro() + 16 * nfr()) [[unlikely]]
        return false;

    // Check if all registers at IRO are covered by the MMIO region
    if (mmio_size < iro() + 16) [[unlikely]]
        return false;

    // Configure SMMU fault interrupt
    write (Reg32::FEUADDR, static_cast<uint32_t>(addr >> 32));
    write (Reg32::FEADDR,  static_cast<uint32_t>(addr));
    write (Reg32::FEDATA, VEC_FLT);
    write (Reg32::FECTL, read (Reg32::FECTL) & ~BIT (31));

    // Clear any faults that earlier boot stages may have caused
    clear_faults();

    // Enable INV first because DEV/IRT depend on it
    if (!init_inv() || !init_dev() || !init_irt() || !init_pmr()) [[unlikely]]
        return false;

    auto const ver { read (Reg32::VER) };

    trace (TRACE_SMMU, "SMMU: %#010lx %#lx %u.%u SEG:%#06x LEV:%u (%s Mode)",
           phys, mmio_size, ver >> 4 & BIT_RANGE (3, 0), ver & BIT_RANGE (3, 0), Pci::seg (sbdf),
           lev(), noir ? "NoIR" : x2apic ? "x2APIC" : "xAPIC");

    return true;
}

Status Smmu_itl::assign_dev (pci_t const src, Space_dma *o, Space_dma *n, uintptr_t &sbw)
{
    assert (o || n);

    // Source device and IOMMU must be in the same SEG
    if (Pci::seg (src) != Pci::seg (sbdf)) [[unlikely]]
        return Status::BAD_DEV;

    // Determine PTAB level for this SMMU
    auto const ptl { min (Dpt_itl::lev(), lev()) };

    // PTL cannot exceed 3 bits
    assert (ptl < BIT (3));

    // Determine PTAB root pointers
    auto const ptr_o { o ? o->get_root_itl (ptl) : nullptr };
    auto const ptr_n { n ? n->get_root_itl (ptl) : nullptr };

    // Unable to lookup/allocate PTAB root
    if ((o && !ptr_o) || (n && !ptr_n)) [[unlikely]]
        return Status::MEM_OBJ;

    // Unable to lookup/allocate entry
    auto const entry { dtbl->entry (src) };
    if (!entry) [[unlikely]]
        return Status::MEM_OBJ;

    // Acquire reference for n
    if (n && !n->try_inc()) [[unlikely]]
        return Status::ABORTED;

    auto cte_o { o ? Devtable::Cte { ptr_o, ptl, o->get_sdid() } : Devtable::Cte{} };
    auto cte_n { n ? Devtable::Cte { ptr_n, ptl, n->get_sdid() } : Devtable::Cte{} };

    // Linearization Point
    if (!entry->compare_exchange (cte_o, cte_n, Dpt_itl::noncoherent)) [[unlikely]] {

        // Release unused reference for n
        if (n) [[likely]]
            n->ref_dec();

        // Report incorrect o
        return Status::BAD_PAR;
    }

    // Invalidation is required if the CTE changed (and it was previously valid or CM is active)
    if (o != n && (o || feature (Cap::CM))) [[unlikely]] {

        // Removing a device from a domain (o || CM) also requires invalidating BDF:DOM-tagged TLB entries (CM tags invalid entries with DOM=0)
        auto const ok { Smmu::seg_invalidate<Smmu_itl> (Pci::seg (src), [bdf = Pci::bdf (src), dom = cte_o.dom()] (auto smmu) { return smmu->invalidate_ctx (bdf, dom); }) };

        if (!ok) [[unlikely]] {
            trace (TRACE_ERROR, "SMMU: %#010lx Invalidation Timeout (Faulty HW?)", phys);
            return Status::TIMEOUT;
        }
    }

    // Release reference for o only if SMMU invalidation succeeded
    if (o) [[unlikely]]
        o->ref_dec();

    if (n) [[likely]]
        trace (TRACE_SMMU, "SMMU: %#010lx Assigned %04x:%02x:%02x.%x to Domain %#06x", phys, Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src), n->get_sdid());

    // Effective selector bit width for this SMMU
    sbw = min (Dpt_itl::ibits - PAGE_BITS, Dpt_itl::bpl * ptl);

    return Status::SUCCESS;
}

void Smmu_itl::interrupt()
{
    auto const fsts { read (Reg32::FSTS) };

    if (fsts & Fault::PPF) [[unlikely]] {
        uint128_t val;
        for (unsigned frr { fsts >> 8 & BIT_RANGE (7, 0) }; read_frr (frr, val); frr = (frr + 1) % nfr()) {
            auto const src { Pci::pci (Pci::seg (sbdf), static_cast<uint16_t>(val >> 64)) };
            trace (TRACE_SMMU, "SMMU: %#010lx SRC:%04x:%02x:%02x.%x FR:%#04x FI:%#010lx", phys, Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src), static_cast<uint8_t>(val >> 96), static_cast<uint64_t>(val));
        }
    }

    // Queued Invalidation Interface Errors
    if (fsts & (Fault::ITE | Fault::ICE | Fault::IQE)) [[unlikely]] {

        auto const error { read (Reg64::IQERCD) };

        // Invalidation Timeout Error
        if (fsts & Fault::ITE) [[unlikely]] {
            auto const src { Pci::pci (Pci::seg (sbdf), static_cast<uint16_t>(error >> 32)) };
            trace (TRACE_SMMU, "SMMU: %#010lx ITE from %04x:%02x:%02x.%x", phys, Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src));
        }

        // Invalidation Completion Error
        if (fsts & Fault::ICE) [[unlikely]] {
            auto const src { Pci::pci (Pci::seg (sbdf), static_cast<uint16_t>(error >> 48)) };
            trace (TRACE_SMMU, "SMMU: %#010lx ICE from %04x:%02x:%02x.%x", phys, Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src));
        }

        // Invalidation Queue Error
        if (fsts & Fault::IQE) [[unlikely]] {
            Inv_iwt inv;
            invq_repair (inv);
            trace (TRACE_SMMU, "SMMU: %#010lx Bad Invalidation %#x (%lu)", phys, std::to_underlying (inv.type()), error & BIT_RANGE (3, 0));
        }
    }

    // Clear all RW1C fault bits
    clear_faults();
}
