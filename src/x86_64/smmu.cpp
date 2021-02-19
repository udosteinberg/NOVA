/*
 * System Memory Management Unit (Intel IOMMU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "pd.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu::cache { sizeof (Smmu), alignof (Smmu) };
INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu::Grp::cache { sizeof (Smmu::Grp), alignof (Smmu::Grp) };

Smmu::Smmu (uint64_t p, Grp *g, Inv *q) : List { list }, Mmio { p, PAGE_SIZE (0) }, cap { read (Reg64::CAP) }, ecap { read (Reg64::ECAP) }, grp { g }, inv { q }
{
    // Set DPT maximum leaf level
    Dptp::set_mll (mll());

    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt::noncoherent |= !feature (Ecap::PWC);

    // If the SMMU does not support interrupt remapping, then disable it
    ir &= feature (Ecap::IR);

    // If the SMMU does not support x2APIC, then disable it
    Lapic::x2apic &= feature (Ecap::EIM);

    auto const ver { read (Reg32::VER) };

    trace (TRACE_SMMU, "SMMU: %#010lx %u.%u SEG:%#06x CAP:%#018lx ECAP:%#018lx LEV:%u MLL:%u", phys, ver >> 4 & BIT_RANGE (3, 0), ver & BIT_RANGE (3, 0), grp->seg, cap, ecap, lev(), mll());
}

Smmu *Smmu::setup (uint64_t phys, uint16_t seg)
{
    // Allocate segment group
    auto const grp { Grp::setup (seg) };
    if (!grp) [[unlikely]]
        return nullptr;

    // Allocate invalidation queue
    auto const inv { static_cast<Inv *>(Buddy::alloc (ord, Buddy::Fill::BITS0)) };
    if (!inv) [[unlikely]]
        return nullptr;

    // Allocate SMMU
    auto const smmu { new Smmu { phys, grp, inv } };
    if (!smmu) [[unlikely]]
        return nullptr;

    return smmu;
}

void Smmu::init()
{
    apic_t const dst { Lapic::id[0] };

    // Configure SMMU fault interrupt
    write (Reg32::FEUADDR, dst & BIT_RANGE (31, 8));
    write (Reg32::FEADDR, Lapic::msi_base | (dst & BIT_RANGE (7, 0)) << 12);
    write (Reg32::FEDATA, VEC_FLT);
    write (Reg32::FECTL, 0);

    // Clear any pending faults that may have occurred in prior boot stages
    write (Reg32::FSTS, Fault::ITE | Fault::ICE | Fault::IQE | Fault::APF | Fault::AFO | Fault::PFO);

    init_inv();
    init_irt();
    init_ctx();
    init_pmr();
}

Status Smmu::assign_dev (Pd *p, uintptr_t dad, bool invalidate)
{
    // Determine src device
    auto const src { static_cast<pci_t>(dad) };

    // Check that src device and IOMMU are in the same PCI segment group
    if (Pci::seg (src) != grp->seg) [[unlikely]]
        return Status::BAD_DEV;

    auto const rlev { lev() };
    auto const sdid { p->get_sdid() };
    auto const ptab { p->dpt.root_init (rlev - 1) };

    if (!ptab) [[unlikely]]
        return Status::BAD_PAR;

    // Determine root table slot
    auto const ctx { grp->ctx + Pci::bus (src) };

    uint16_t zap;

    {   Lock_guard <Spinlock> guard { cfg_lock };

        if (!ctx->present())
            ctx->set (0, Kmem::ptr_to_phys (new Entry_ctx) | BIT (0));

        auto const c { static_cast<Entry_ctx *>(Kmem::phys_to_ptr (ctx->addr())) + Pci::ari (src) };

        zap = c->did();

        if (!c->present())
            invalidate = false;
        else
            c->set (0, 0);

        c->set (sdid << 8 | (rlev - 2), Kmem::ptr_to_phys (ptab) | BIT (0));
    }

    if (invalidate) [[likely]]
        invalidate_ctx (Pci::bdf (src), zap);

    trace (TRACE_SMMU, "SMMU: %#010lx Device %04x:%02x:%02x.%x assigned to Domain %u", phys, Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src), static_cast<unsigned>(sdid));

    return Status::SUCCESS;
}

Status Smmu::assign_int (Entry_irt *irt, iid_t iid, cpu_t cpu, uint8_t vec, pci_t src, uint8_t cfg, uintptr_t &msi_addr, uintptr_t &msi_data)
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

    // Override src for pin-based interrupts
    if (ioapic) [[unlikely]]
        src = ioapic->src();

    // Check flags for message-signaled interrupts
    else if (cfg & BIT_RANGE (3, 0)) [[unlikely]]
        return Status::BAD_PAR;

    // Convert interrupt to SEG:GSI
    auto const seg { Intid::to_seg (iid) };
    auto const gsi { Intid::to_gsi (iid) };

    // Check that src device and interrupt are in the same PCI segment group
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
        all_invalidate_iec (seg, gsi);

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

void Smmu::fault()
{
    auto const fsts { read (Reg32::FSTS) };

    if (fsts & Fault::PPF) [[unlikely]] {
        uint64_t hi, lo;
        for (unsigned frr { fsts >> 8 & BIT_RANGE (7, 0) }; read (frr, hi, lo), hi & BIT64 (63); frr = (frr + 1) % nfr()) {
            pci_t const src { static_cast<uint16_t>(hi) };
            trace (TRACE_SMMU, "SMMU: %#010lx FRR:%u FR:%#x SRC:%02x:%02x.%x FI:%#010lx", phys, frr, static_cast<uint8_t>(hi >> 32), Pci::bus (src), Pci::dev (src), Pci::fun (src), lo);
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

    write (Reg32::FSTS, Fault::ITE | Fault::ICE | Fault::IQE | Fault::APF | Fault::AFO | Fault::PFO);
}
