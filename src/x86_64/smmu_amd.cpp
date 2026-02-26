/*
 * System Memory Management Unit (AMD IOMMU)
 *
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

#include "dc.hpp"
#include "smmu_amd.hpp"
#include "space_dma.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu_amd::cache { sizeof (Smmu_amd), alignof (Smmu_amd) };

uint64_t Smmu_amd::sequence {}, Smmu_amd::doorbell {};

// SMMU occupies a naturally-aligned 16K MMIO region (512K if EFR1[PCSup] = 1)
Smmu_amd::Smmu_amd (uint64_t p, pci_t t, uint64_t e1, uint64_t e2, Devtable *d) : Smmu { p, 0x4000, t }, efr1 { e1 }, efr2 { e2 }, dtbl { d }, cmdq { ord_cmd }, evtq { ord_evt }
{
    // If the SMMU does not support 128-bit IRTE, then disable IR globally
    noir |= !feat_ga();

    // Operate in x2APIC mode only if the LAPICs and the SMMU support it
    x2apic &= Lapic::x2apic && feat_xt();
}

Smmu_amd *Smmu_amd::create (uint64_t phys, pci_t sbdf, uint64_t efr1, uint64_t efr2)
{
    // Set SMMU list type to AMD. Success makes the list downcast safe
    if (!set_type (Type::AMD)) [[unlikely]]
        return nullptr;

    // Look for sibling SMMU in the same PCI segment group
    auto const sibling { static_cast<Smmu_amd *>(lookup_seg (Pci::seg (sbdf))) };

    // Share per-segment data structures with siblings
    auto const dtbl { sibling ? sibling->dtbl : new Devtable };

    // Allocate SMMU
    auto const smmu { new Smmu_amd { phys, sbdf, efr1, efr2, dtbl } };

    // Add to SMMU list only if all structure allocations succeeded
    if (smmu && smmu->dtbl && smmu->cmdq.ptr && smmu->evtq.ptr) [[likely]] {
        smmu->insert (list);
        return smmu;
    }

    return nullptr;
}

bool Smmu_amd::init()
{
    // Ensure all data structures are present
    assert (dtbl && cmdq.ptr && evtq.ptr);

    // First SMMU in each PCI segment adjusts the device table if IR is disabled
    if (noir && lookup_seg (Pci::seg (sbdf)) == this) [[unlikely]]
        if (!dtbl->noir())
            return false;

    // Lookup associated PCI function
    auto const pci { Pci::Function::lookup (sbdf) };
    if (!pci || !pci->cap<Pci::Cap_sdev>()) [[unlikely]]
        return false;

    // Check if capability layout is IOMMU
    auto const hdr { pci->read (Pci::Cap_sdev::Reg32::HDR) };
    if ((hdr >> 16 & BIT_RANGE (2, 0)) != 3) [[unlikely]]
        return false;

    // Treat DPT as caching not-present entries if at least one SMMU requires it
    Dpt_amd::inv_notpresent |= hdr & BIT (26);

    // Check if ACPI-reported features are consistent with hardware
    if ((hdr & BIT (27)) && (read (Reg64::EFR1) != efr1 || read (Reg64::EFR2) != efr2)) [[unlikely]]
        return false;

    // Check if HATS reports the reserved 11b encoding
    if ((efr1 >> 10 & BIT_RANGE (1, 0)) == BIT_RANGE (1, 0)) [[unlikely]]
        return false;

    // Check if BAR matches IOMMU physical address and is enabled/locked
    auto const bar { uint64_t { pci->read (Pci::Cap_sdev::Reg32::BAR_HI) } << 32 | pci->read (Pci::Cap_sdev::Reg32::BAR_LO) };
    if ((bar & (BITN_RANGE (63, 14) | BIT (0))) != (phys | BIT (0))) [[unlikely]]
        return false;

    // Disable IOMMU to facilitate reprogramming the settings from earlier boot stages
    if (!control (0, STS::CMD_RUN | STS::EVT_RUN, 0)) [[unlikely]]
        return false;

    if (x2apic) [[likely]] {

        // x2APIC destination: ambient CPU (BSP)
        auto const icr { uint64_t { VEC_FLT } << 32 | uint64_t { Cpu::topology & BIT_RANGE (31, 24) } << 32 | uint32_t { Cpu::topology & BIT_RANGE (23, 0) } << 8 };

        // Program all interrupts identically
        write (Reg64::ICR_EVT, icr);
        write (Reg64::ICR_PPR, icr);
        write (Reg64::ICR_GVA, icr);

    } else if (!pci->configure_msi (Lapic::msi_addr (Cpu::id), VEC_FLT)) [[unlikely]]
        return false;

    // Configure DEV table
    write (Reg64::DEV_TBL_BASE, Kmem::ptr_to_phys (dtbl) | (BIT (ord_dev) - 1));

    // Configure CMD queue (requires STS::CMD_RUN = 0, sets Reg64::CMD_HEAD = Reg64::CMD_TAIL = 0)
    write (Reg64::CMD_BUF_BASE, uint64_t { cmdq.ord } << 56 | cmdq.base());

    // Configure EVT queue (requires STS::EVT_RUN = 0, sets Reg64::EVT_HEAD = Reg64::EVT_TAIL = 0)
    write (Reg64::EVT_LOG_BASE, uint64_t { evtq.ord } << 56 | evtq.base());

    // Disable exclusion range
    write (Reg64::EXC_RNG_BASE, 0);

    // Disable GPA=HPA bypass
    if (feat_po()) [[likely]]
        write (Reg32::PERF_OPT_CTRL, 0);

    // Enable IOMMU
    if (!control (x2apic * (CTL::E_XT_INT | CTL::E_XT) | !Smmu::noir * CTL::E_GA | CTL::E_CMD_BUF | CTL::E_COHERENT | CTL::E_INT_EVT | CTL::E_EVT_LOG | CTL::E_IOMMU, STS::CMD_RUN | STS::EVT_RUN, STS::CMD_RUN | STS::EVT_RUN)) [[unlikely]]
        return false;

    trace (TRACE_SMMU, "SMMU: %#010lx %#lx %04x:%02x:%02x.%x LEV:%u (%s Mode)",
           phys, mmio_size, Pci::seg (sbdf), Pci::bus (sbdf), Pci::dev (sbdf), Pci::fun (sbdf),
           lev(), noir ? "NoIR" : x2apic ? "x2APIC" : "xAPIC");

    // Invalidate all IOMMU caches
    return invalidate_all();
}

void Smmu_amd::interrupt()
{
    // Snapshot SMMU status because HW can change it concurrently
    auto const s { read (Reg64::STS) };

    // Handle event log (first clear the interrupt for new events, then consume existing events)
    if (s & STS::EVT_INT) [[likely]] {
        write (Reg64::STS, STS::EVT_INT);
        handle_events();
    }

    // Restart CMD_BUF (if halted) / EVT_LOG (if overflowed)
    restart (CTL::E_CMD_BUF * !(s & STS::CMD_RUN) | CTL::E_EVT_LOG * (s & STS::EVT_OVR));
}

void Smmu_amd::event (Evt const &e)
{
    switch (e.type()) {

        default:
            trace (TRACE_SMMU, "SMMU: %#010lx EVT:%#x OP1:%#lx OP2:%#lx", phys, std::to_underlying (e.type()), e.eop1(), e.eop2());
            break;

        // CMDQ halts in these two cases, which facilitates replacing the bad command with a COMPLETION_WAIT
        case Evt::Type::ILLEGAL_COMMAND_ERROR:
        case Evt::Type::COMMAND_HARDWARE_ERROR:
            Cmd_completion_wait cmd;
            cmdq_repair (cmd);
            trace (TRACE_SMMU, "SMMU: %#010lx Bad Command %#x", phys, std::to_underlying (cmd.type()));
            break;
    }
}

Status Smmu_amd::assign_dev (pci_t const src, Space_dma *o, Space_dma *n, uintptr_t &sbw)
{
    assert (o || n);

    // Source device and IOMMU must be in the same SEG
    if (Pci::seg (src) != Pci::seg (sbdf)) [[unlikely]]
        return Status::BAD_DEV;

    // Determine PTAB level for this SMMU
    auto const ptl { min (Dpt_amd::lev(), lev()) };

    // PTL cannot exceed 3 bits
    assert (ptl < BIT (3));

    // Determine PTAB root pointers
    auto const ptr_o { o ? o->get_root_amd (ptl) : nullptr };
    auto const ptr_n { n ? n->get_root_amd (ptl) : nullptr };

    // Unable to lookup/allocate PTAB root
    if ((o && !ptr_o) || (n && !ptr_n)) [[unlikely]]
        return Status::MEM_OBJ;

    // Unable to lookup entry
    auto const entry { dtbl->cte (src) };
    if (!entry) [[unlikely]]
        return Status::BAD_DEV;

    // Acquire reference for n
    if (n && !n->try_inc()) [[unlikely]]
        return Status::ABORTED;

    auto cte_o { o ? Devtable::Cte { ptr_o, ptl, o->get_sdid() } : Devtable::Cte{} };
    auto cte_n { n ? Devtable::Cte { ptr_n, ptl, n->get_sdid() } : Devtable::Cte{} };

    // Linearization Point
    if (!entry->compare_exchange (cte_o, cte_n, false)) [[unlikely]] {

        // Release unused reference for n
        if (n) [[likely]]
            n->ref_dec();

        // Report incorrect o
        return Status::BAD_PAR;
    }

    // Invalidation is required if the CTE changed (even if the CTE half was previously invalid, the ITE half can cause it to be cached)
    if (o != n) {

        // Removing a device from a domain (o) also requires invalidating BDF:DOM-tagged TLB entries
        auto const ok { o ? Smmu::seg_invalidate<Smmu_amd> (Pci::seg (src), [bdf = Pci::bdf (src), dom = cte_o.dom()] (auto smmu) { return smmu->invalidate_dte (bdf, dom); })
                          : Smmu::seg_invalidate<Smmu_amd> (Pci::seg (src), [bdf = Pci::bdf (src)]                    (auto smmu) { return smmu->invalidate_dte (bdf); }) };

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
    sbw = min (Dpt_amd::ibits - PAGE_BITS, Dpt_amd::bpl * ptl);

    return Status::SUCCESS;
}

bool Smmu_amd::nova_assigned (pci_t const src)
{
    auto const e { dtbl->cte (src) };

    return e && Devtable::Cte { e->load() }.dom() == Space_dma::nova.get_sdid();
}
