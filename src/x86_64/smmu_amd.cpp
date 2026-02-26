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

uint64_t Smmu_amd::sequence, Smmu_amd::doorbell;

// SMMU occupies a naturally-aligned 16K MMIO region (512K if 0030h[PCSup] = 1)
Smmu_amd::Smmu_amd (uint64_t p, pci_t t, uint64_t e1, uint64_t e2, Devtable *d) : Smmu { p, 0x4000, t }, efr1 { e1 }, efr2 { e2 }, dtbl { d }, cmdq { ord_cmd }, evtq { ord_evt }
{
    // If the SMMU does not support x2APIC, then disable it globally
    Lapic::x2apic &= feat_xt();
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

    // Lookup associated PCI function
    auto const pci { Pci::Function::lookup (sbdf) };
    if (!pci || !pci->cap<Pci::Cap_sdev>()) [[unlikely]]
        return false;

    // Check if capability layout is IOMMU
    auto const hdr { pci->read (Pci::Cap_sdev::Reg32::HDR) };
    if ((hdr >> 16 & BIT_RANGE (2, 0)) != 3) [[unlikely]]
        return false;

    // Check if ACPI-reported features are consistent with hardware
    if ((hdr & BIT (27)) && (read (Reg64::EFR1) != efr1 || read (Reg64::EFR2) != efr2)) [[unlikely]]
        return false;

    // Check if BAR matches IOMMU physical address and is enabled/locked
    auto const bar { uint64_t { pci->read (Pci::Cap_sdev::Reg32::BAR_HI) } << 32 | pci->read (Pci::Cap_sdev::Reg32::BAR_LO) };
    if ((bar & (BITN_RANGE (63, 14) | BIT (0))) != (phys | BIT (0))) [[unlikely]]
        return false;

    // Obtain IOMMU info
    auto const rng { pci->read (Pci::Cap_sdev::Reg32::RANGE) };
    auto const mi0 { pci->read (Pci::Cap_sdev::Reg32::MISC_0) };

    // Determine VA/PA size and MSI number
    auto const vas { BIT_RANGE (6, 0) & mi0 >> 15 };
    auto const pas { BIT_RANGE (6, 0) & mi0 >>  8 };
    auto const msi { BIT_RANGE (4, 0) & mi0 };

    if (Lapic::x2apic) [[likely]] {

        // x2APIC destination: ambient CPU (BSP)
        auto const icr { uint64_t { VEC_FLT } << 32 | uint64_t { Cpu::topology & BIT_RANGE (31, 24) } << 32 | uint32_t { Cpu::topology & BIT_RANGE (23, 0) } << 8 };

        // Program all interrupts identically
        write (Reg64::ICR_EVT, icr);
        write (Reg64::ICR_PPR, icr);
        write (Reg64::ICR_GVA, icr);

    } else if (!pci->configure_msi (Lapic::msi_addr (Cpu::id), VEC_FLT)) [[unlikely]]
        return false;

    // Configure DEV table
    write (Reg64::DEV_TBL_BASE, dtbl->base() | (BIT (ord_dev) - 1));

    // Configure CMD queue (implicitly sets CMD_HEAD = CMD_TAIL = 0)
    write (Reg64::CMD_BUF_BASE, uint64_t { cmdq.ord } << 56 | cmdq.base());

    // Configure EVT queue (implicitly sets EVT_HEAD = EVT_TAIL = 0)
    write (Reg64::EVT_LOG_BASE, uint64_t { evtq.ord } << 56 | evtq.base());

    // Disable exclusion range
    write (Reg64::EXC_RNG_BASE, 0);

    // Disable GPA=HPA bypass
    if (efr1 & BITN (45)) [[likely]]
        write (Reg32::PERF_OPT_CTRL, 0);

    // Enable IOMMU
    if (!control (Lapic::x2apic * (CTL::E_XT_INT | CTL::E_XT | CTL::E_GA) | CTL::E_CMD_BUF | CTL::E_COHERENT | CTL::E_INT_EVT | CTL::E_EVT_LOG | CTL::E_IOMMU, STS::CMD_RUN | STS::EVT_RUN)) [[unlikely]]
        return false;

    trace (TRACE_SMMU, "SMMU: %#010lx %04x:%02x:%02x.%x DEV:%#lx CMD:%#lx EVT:%#lx LEV:%u VAS:%u PAS:%u MSI:%u RNG:%#x",
           phys, Pci::seg (sbdf), Pci::bus (sbdf), Pci::dev (sbdf), Pci::fun (sbdf),
           dtbl->base(), cmdq.base(), evtq.base(), lev(), vas, pas, msi, rng);

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

    trace (TRACE_SMMU, "SMMU: %#010lx Status %#lx->%#lx", phys, s, read (Reg64::STS));
}

void Smmu_amd::event (Evt const &e)
{
    switch (e.type()) {

        default:
            trace (TRACE_SMMU, "SMMU: %#010lx EVT:%#x OP1:%#lx OP2:%#lx", phys, std::to_underlying (e.type()), e.eop1(), e.eop2());
            break;

        // CMDQ halts in these two cases, which facilitates replacing the command with a COMPLETION_WAIT
        case Evt::Type::ILLEGAL_COMMAND_ERROR:
        case Evt::Type::COMMAND_HARDWARE_ERROR:
            Cmd_completion_wait cmd;
            cmdq.replace (cmdq_head(), cmd);
            trace (TRACE_SMMU, "SMMU: %#010lx Bad Command %#x", phys, std::to_underlying (cmd.type()));
            break;
    }
}

Status Smmu_amd::assign_dev (Dc const *dc, Space_dma *dma, bool invalidate)
{
    // Obtain source device from DC
    auto const src { dc->sbdf };

    // Source device and IOMMU must be in the same SEG and BDF must fit into Devtable
    if (Pci::seg (src) != Pci::seg (sbdf) || Pci::bdf (src) >= dtbl->size()) [[unlikely]]
        return Status::BAD_DEV;

    // Determine PTAB level, PTAB root, domain ID
    auto const ptl { min (Dpt_amd::lev(), lev()) };
    auto const ptr { dma->get_root_amd (ptl) };
    auto const dom { dma->get_dom() };

    // PTL cannot exceed 3 bits
    assert (ptl < BIT (3));

    // Unable to lookup/allocate PTAB root
    if (!ptr) [[unlikely]]
        return Status::MEM_OBJ;

    // Update DTE with a single 128-bit write and return the prior value
    auto const dte { dtbl->set_dev (Pci::bdf (src), dom, ptr, ptl) };
    auto const old { static_cast<uint16_t>(dte >> 64) };

    // Skip invalidation if the DTE was previously invalid or the domain did not change
    if (!(dte & BIT (0)) || old == dom) [[unlikely]]
        invalidate = false;

    // Invalidate IOMMU caches
    if (invalidate) [[likely]]
        if (!Smmu::seg_invalidate_dev (old, src)) [[unlikely]] {
            trace (TRACE_ERROR, "SMMU: %#010lx Invalidation Timeout (Faulty HW?)", phys);
            return Status::TIMEOUT;
        }

    trace (TRACE_SMMU, "SMMU: %#010lx Assigned %04x:%02x:%02x.%x to Domain %u", phys, Pci::seg (src), Pci::bus (src), Pci::dev (src), Pci::fun (src), dom);

    return Status::SUCCESS;
}
