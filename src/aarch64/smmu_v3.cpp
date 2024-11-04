/*
 * System Memory Management Unit (Arm SMMUv3)
 *
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

#include "dc.hpp"
#include "gicd.hpp"
#include "smmu_v3.hpp"
#include "space_dma.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu_v3::cache { sizeof (Smmu_v3), alignof (Smmu_v3) };

// SMMU registers occupy two consecutive 64KB pages starting from an at least 64KB-aligned boundary
Smmu_v3::Smmu_v3 (uint64_t base, arm_intid_t const (&i)[4]) : Smmu { base, 0x20000 }, intid { i[0], i[1], i[2], i[3] },
                  idr0 { read (Reg32::IDR0) }, idr1 { read (Reg32::IDR1) }, idr5 { read (Reg32::IDR5) },
                  cmdq { coherent(), hwo_cmd() },
                  evtq { coherent(), hwo_evt() },
                  strt { coherent(), hwo_sid() }
{
    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt::noncoherent |= !coherent();

    auto const aidr { read (Reg32::AIDR) };
    auto const iidr { read (Reg32::IIDR) };

    trace (TRACE_SMMU, "SMMU: %#010lx %03x:%03x r%up%u v3.%u P:%u S:%u C:%u I:%#x/%#x/%#x/%#x",
           phys, iidr & BIT_RANGE (11, 0), iidr >> 20, iidr >> 16 & BIT_RANGE (3, 0), iidr >> 12 & BIT_RANGE (3, 0),
           aidr & BIT_RANGE (7, 0), Dpt::pas (oas()), strt.ord, coherent(), intid[0], intid[1], intid[2], intid[3]);
}

bool Smmu_v3::init()
{
    // Setup SMMU interrupts
    for (unsigned i { 0 }; i < sizeof (intid) / sizeof (*intid); i++)
        if (Intid::type (intid[i]) == Intid::Type::SPI) [[likely]]
            Gicd::conf_std (intid[i], Cpu::id);

    // Configure global bypass to abort all incoming transactions
    if (!set_gbpa (BIT (20))) [[unlikely]]
        return false;

    // Disable SMMU to facilitate configuration
    if (!set_cr0 (0)) [[unlikely]]
        return false;

    // Disable interrupts to facilitate configuration
    if (!set_irq_ctrl (0)) [[unlikely]]
        return false;

    // Ensure stage-2 translation, VMSAv8 format and 4K translation granule are supported
    if (!feat_s2p() || !feat_t64() || !feat_g04()) [[unlikely]]
        return false;

    // Ensure hardwired storage is not being used
    if (idr1 & BIT_RANGE (30, 29)) [[unlikely]]
        return false;

    // Ensure data structure allocations succeeded
    if (!cmdq.ptr || !evtq.ptr || !strt.ptr) [[unlikely]]
        return false;

    // Acknowledge all prior global errors
    write (Reg32::GERRORN, read (Reg32::GERROR));

    // Configure interrupts
    if (feat_msi()) [[likely]] {

        // Disable GERROR MSI (requires IRQ_CTRL.GERROR_IRQEN == 0)
        write (Reg64::GERROR_IRQ_CFG0, 0);      // MSI Addr
        write (Reg32::GERROR_IRQ_CFG1, 0);      // MSI Data
        write (Reg32::GERROR_IRQ_CFG2, 0);      // Memory Type, Shareability

        // Disable EVENTQ MSI (requires IRQ_CTRL.EVENTQ_IRQEN == 0)
        write (Reg64::EVENTQ_IRQ_CFG0, 0);      // MSI Addr
        write (Reg32::EVENTQ_IRQ_CFG1, 0);      // MSI Data
        write (Reg32::EVENTQ_IRQ_CFG2, 0);      // Memory Type, Shareability
    }

    // Enable supported interrupts
    if (!set_irq_ctrl (BIT (2) | feat_pri() * BIT (1) | BIT (0))) [[unlikely]]
        return false;

    // Queue indexes may be non-zero upon resume
    auto const cmdq_idx { cmdq.get_idx() };
    auto const evtq_idx { evtq.get_idx() };

    // Configure commmand queue (requires CR0.CMDQEN == 0)
    write (Reg64::CMDQ_BASE, BIT64 (62) | cmdq.get_ptr() | cmdq.ord);       // Affects PROD/CONS
    write (Reg32::CMDQ_PROD, cmdq_idx);
    write (Reg32::CMDQ_CONS, cmdq_idx);

    // Configure event queue (requires CR0.EVENTQEN == 0)
    write (Reg64::EVENTQ_BASE, BIT64 (62) | evtq.get_ptr() | evtq.ord);     // Affects PROD/CONS
    write (Reg32::EVENTQ_PROD, evtq_idx);
    write (Reg32::EVENTQ_CONS, evtq_idx);

    // Configure stream table (requires CR0.SMMUEN == 0)
    write (Reg32::STRTAB_BASE_CFG, strt.get_cfg());                         // Affects BASE
    write (Reg64::STRTAB_BASE, BIT64 (62) | strt.get_ptr());

    // Configure CR2 (requires CR0.SMMUEN == 0)
    write (Reg32::CR2, feat_btm() * BIT (2) | BIT (1));

    // Configure CR1 (requires CR0.SMMUEN == 0 and CR0.*QEN == 0)
    write (Reg32::CR1, BIT_RANGE (11, 10) | BIT (8) | BIT_RANGE (6, 4) | BIT (2) | BIT (0));

    // Enable CMDQ + EVTQ
    if (!set_cr0 (BIT_RANGE (3, 2))) [[unlikely]]
        return false;

    // Invalidate all cached configuration and EL2/EL1 TLB entries (see 3.11)
    if (!cfgi_all() || !tlbi_alle2() || !tlbi_alle1()) [[unlikely]]
        return false;

    // Enable CMDQ + EVTQ + SMMU
    return set_cr0 (BIT_RANGE (3, 2) | BIT (0));
}

Status Smmu_v3::assign_dev (Dc const *dc, Space_dma *dma)
{
    auto const sid { dc->sid };

    // Check if SID is within range supported by the stream table
    if (sid >= BIT64 (strt.ord)) [[unlikely]]
        return Status::BAD_PAR;

    // CFG invalidation lambdas
    auto const stdi = [&]() { return cfgi_ste (sid, false); };
    auto const stei = [&]() { return cfgi_ste (sid, true); };

    // Lookup (allocate) stream table entry for the specified SID
    auto const ste { strt.entry (coherent(), stdi, sid) };

    // Check if stream table entry has been allocated
    if (!ste) [[unlikely]]
        return Status::MEM_OBJ;

    // TLB invalidation lambda
    auto tlbi = [&]() { return tlbi_vmalls12e1 (ste->vmid()); };

    Status s;

    // Detach STE
    if ((s = ste->detach (coherent(), stei, tlbi)) != Status::SUCCESS) [[unlikely]]
        return s;

    // We're done if there is no DMA space to attach
    if (!dma) [[unlikely]]
        return Status::SUCCESS;

    auto const pas { oas() };                               // VMSAv8-64 IPA = OAS
    auto const isz { min (Dpt::pas (pas), Dpt::ibits) };    // Input address size
    auto const lev { Dpt::lev (isz) };                      // Number of levels

    // Lookup (allocate) page table root
    auto const ptab { dma->get_ptab (lev - 1) };
    auto const vmid { dma->get_sdid() };

    // Check if page table root has been allocated
    if (!ptab) [[unlikely]]
        return Status::MEM_OBJ;

    // Attach STE
    if ((s = ste->attach (coherent(), stei, ptab, vmid, pas, isz, lev)) != Status::SUCCESS) [[unlikely]]
        return s;

    trace (TRACE_SMMU, "SMMU: %#lx Assign SID:%#010x to VMID:%u PAS:%u ISZ:%u LEV:%u", phys, sid, static_cast<uint16_t>(vmid), pas, isz, lev);

    return Status::SUCCESS;
}

void Smmu_v3::handle_evt (uint32_t const hwi)
{
    while (!evtq.is_empty (hwi)) [[likely]] {

        Evt evt;

        // Get event
        evtq.consume (evt);

        // Decode event
        switch (evt.type()) {

            case Evt::Type::C_BAD_STREAMID:
                trace (TRACE_ERROR, "SMMU: %#lx C_BAD_STREAMID SID:%#x", phys, evt.sid());
                break;

            case Evt::Type::C_BAD_STE:
                trace (TRACE_ERROR, "SMMU: %#lx C_BAD_STE SID:%#x", phys, evt.sid());
                break;

            case Evt::Type::F_TRANSLATION:
                trace (TRACE_ERROR, "SMMU: %#lx F_TRANSLATION SID:%#x IADDR:%#lx", phys, evt.sid(), evt.iaddr());
                break;

            default:
                trace (TRACE_ERROR, "SMMU: %#lx Event Type:%#x SID:%#x", phys, std::to_underlying (evt.type()), evt.sid());
                break;
        }
    }
}

void Smmu_v3::handle_glb (uint32_t const err)
{
    /*
     * CMDQ_ERR: A command has been encountered that cannot be processed
     *
     * CMDQ_CONS is r/o while the command queue is enabled, so we cannot
     * skip the infringing command. Instead we replace the command with a
     * benign SYNC command. Updating a consumer-owned entry is safe because
     * the SMMU stops processing commands until CMDQ_ERR is acknowledged.
     */
    if (err & BIT (0)) [[unlikely]] {
        Cmd cmd { Cmd_sync {} };
        auto const c { read (Reg32::CMDQ_CONS) };
        cmdq.replace (c & BIT_RANGE (19, 0), cmd);
        trace (TRACE_ERROR, "SMMU: %#lx CMD Queue Error CMD %#x ERR %#x", phys, cmd.type(), c >> 24 & BIT_RANGE (6, 0));
    }

    if (err & BIT_RANGE (31, 1)) [[unlikely]]
        trace (TRACE_ERROR, "SMMU: %#lx Global Error %#x", phys, err);
}
