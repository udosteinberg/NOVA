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
#include "ioapic.hpp"
#include "smmu_itl.hpp"
#include "space_dma.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu_itl::cache { sizeof (Smmu_itl), alignof (Smmu_itl) };

uint32_t Smmu_itl::sequence, Smmu_itl::doorbell;

Smmu_itl::Smmu_itl (uint64_t p, pci_t t, Devtable *d, Entry_irt *i) : Smmu { p, PAGE_SIZE (0), t },
                    cap { read (Reg64::CAP) }, ecap { read (Reg64::ECAP) },
                    dtbl { d }, irt { i }, invq { ord_inv }
{
    // Set DPT maximum leaf level
    Dptp_itl::set_mll (mll());

    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt_itl::noncoherent |= !feature (Ecap::PWC);

    // If the SMMU does not support interrupt remapping, then disable it
    ir &= feature (Ecap::IR);

    // If the SMMU does not support x2APIC, then disable it globally
    Lapic::x2apic &= feature (Ecap::EIM);

    auto const ver { read (Reg32::VER) };

    trace (TRACE_SMMU, "SMMU: %#010lx %u.%u SEG:%#06x CAP:%#018lx ECAP:%#018lx LEV:%u MLL:%u", phys, ver >> 4 & BIT_RANGE (3, 0), ver & BIT_RANGE (3, 0), Pci::seg (sbdf), cap, ecap, lev(), mll());
}

Smmu_itl *Smmu_itl::create (uint64_t phys, pci_t sbdf)
{
    // Set SMMU list type to ITL. Success makes the list downcast safe
    if (!set_type (Type::ITL)) [[unlikely]]
        return nullptr;

    // Look for sibling SMMU in the same PCI segment group
    auto const sibling { static_cast<Smmu_itl *>(lookup_seg (Pci::seg (sbdf))) };

    // Share per-segment data structures with siblings
    auto const dtbl { sibling ? sibling->dtbl : new Devtable };
    auto const irt  { sibling ? sibling->irt  : new Entry_irt };

    // Allocate SMMU
    auto const smmu { new Smmu_itl { phys, sbdf, dtbl, irt } };

    // Add to SMMU list only if all structure allocations succeeded
    if (smmu && smmu->dtbl && smmu->irt && smmu->invq.ptr) [[likely]] {
        smmu->insert (list);
        return smmu;
    }

    return nullptr;
}

bool Smmu_itl::init()
{
    auto const addr { Lapic::msi_addr (0) };

    // Configure SMMU fault interrupt
    write (Reg32::FEUADDR, static_cast<uint32_t>(addr >> 32));
    write (Reg32::FEADDR,  static_cast<uint32_t>(addr));
    write (Reg32::FEDATA, VEC_FLT);
    write (Reg32::FECTL, read (Reg32::FECTL) & ~BIT (31));

    // Clear any faults that earlier boot stages may have caused
    clear_faults();

    // Enable INV first because DEV/IRT depend on it
    return init_inv() && init_dev() && init_irt() && init_pmr();
}

Status Smmu_itl::assign_dev (Space_dma *dma, uintptr_t dad, bool invalidate)
{
    // Obtain source device from DAD
    auto const src { static_cast<pci_t>(dad) };

    // Source device and IOMMU must be in the same SEG
    if (Pci::seg (src) != Pci::seg (sbdf)) [[unlikely]]
        return Status::BAD_DEV;

    // Determine PTAB level, PTAB root, domain ID
    auto const ptl { min (Dpt_itl::lev(), lev()) };
    auto const ptr { dma->get_root_itl (ptl) };
    auto const dom { dma->get_dom() };

    // Unable to lookup/allocate PTAB root
    if (!ptr) [[unlikely]]
        return Status::MEM_OBJ;

    // Unable to lookup/allocate DTE
    auto const dte { dtbl->entry (src) };
    if (!dte) [[unlikely]]
        return Status::MEM_OBJ;

    // Replace DTE
    auto const old { dte->update (Devtable::Cte { ptr, ptl, dom }) };

    // Skip invalidation if the old DTE was invalid or the domain did not change
    if (!old.present() || old.dom() == dom)
        invalidate = false;

    if (invalidate) [[likely]]
        if (!Smmu::seg_invalidate_dev (old.dom(), src)) [[unlikely]] {
            trace (TRACE_ERROR, "SMMU: %#010lx Invalidation Timeout (Faulty HW?)", phys);
            return Status::TIMEOUT;
        }

    trace (TRACE_SMMU, "SMMU: %#010lx Assigned %04x:%02x:%02x.%x to Domain %u", phys, Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src), dom);

    return Status::SUCCESS;
}

Status Smmu_itl::assign_int (Entry_irt *irt, iid_t iid, cpu_t cpu, uint8_t vec, pci_t src, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
{
    // Check that cpu is in range
    assert (cpu < Cpu::count);

    // Determine destination ID
    apic_t const dst { Lapic::id[cpu] };

    // Destination ID is limited to 8-bit unless both x2APIC and IR are active
    if (dst >= (Lapic::x2apic && ir ? BIT_RANGE (31, 0) : BIT_RANGE (7, 0))) [[unlikely]]
        return Status::BAD_CPU;

    // Determine IOAPIC
    auto const ioapic { Ioapic::lookup (iid) };

    if (ioapic) [[unlikely]] {

        // Interrupt source is IOAPIC
        src = ioapic->src();

    } else {

        // Flags must be zero
        if (cfg & BIT_RANGE (3, 0)) [[unlikely]]
            return Status::BAD_PAR;
    }

    // Convert interrupt to SEG:GSI
    auto const seg { Intid::to_seg (iid) };
    auto const gsi { Intid::to_gsi (iid) };

    // Device and interrupt must be in the same PCI segment group
    if (Pci::seg (src) != seg) [[unlikely]]
        return Status::BAD_DEV;

    // Extract configuration from flags
    bool const msk { !!(cfg & BIT (0)) };
    bool const trg { !!(cfg & BIT (1)) };
    bool const pol { !!(cfg & BIT (2)) };

    trace (TRACE_INTR, "INTR: Routing GSI %#06x (%c%c%c) from %04x:%02x:%02x.%x to %#06x:%#04x (%s)", gsi, msk ? 'M' : 'U', trg ? 'L' : 'E', pol ? 'L' : 'H', Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src), cpu, vec, ioapic ? "PIN" : "MSI");

    // Populate interrupt remapping table even if IR is not in use
    if (!irt->set (BIT (18) | Pci::bdf (src), static_cast<uint64_t>(dst) << (Lapic::x2apic ? 32 : 40) | vec << 16 | trg << 4 | BIT (0))) [[unlikely]]
        return Status::ABORTED;

    // Invalidate stale cached entries for SEG:GSI
    if (ir) [[likely]]
        if (!Smmu::seg_invalidate_int (seg, gsi)) [[unlikely]]
            return Status::ABORTED;

    // Update IOAPIC
    if (ioapic) [[unlikely]] {
        ioapic->set_dst (gsi, ir ? gsi << 17 | BIT (16) : dst << 24);
        ioapic->set_cfg (gsi, vec, msk, trg, pol);
        msi_addr = msi_data = 0;
    } else {
        msi_addr = Lapic::msi_base | (ir ? BIT_RANGE (4, 3) : dst << 12);
        msi_data = ir ? gsi : vec;
    }

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
            pci_t const src { static_cast<uint16_t>(error >> 32) };
            trace (TRACE_SMMU, "SMMU: %#010lx ITE from SRC:%02x:%02x.%x", phys, Pci::bus (src), Pci::dev (src), Pci::fun (src));
        }

        // Invalidation Completion Error
        if (fsts & Fault::ICE) [[unlikely]] {
            pci_t const src { static_cast<uint16_t>(error >> 48) };
            trace (TRACE_SMMU, "SMMU: %#010lx ICE from SRC:%02x:%02x.%x", phys, Pci::bus (src), Pci::dev (src), Pci::fun (src));
        }

        // Invalidation Queue Error
        if (fsts & Fault::IQE) [[unlikely]]
            trace (TRACE_SMMU, "SMMU: %#010lx IQE %lu", phys, error & BIT_RANGE (3, 0));
    }

    // Clear all RW1C fault bits
    clear_faults();
}
