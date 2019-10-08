/*
 * System Memory Management Unit (Arm SMMUv2)
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
#include "gicd.hpp"
#include "smmu_v2.hpp"
#include "space_dma.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu_v2::cache { sizeof (Smmu_v2), alignof (Smmu_v2) };

Smmu_v2::Smmu_v2 (Board::Smmu_v2 const &brd) : Smmu { brd.mmio, PAGE_SIZE (0) }, idr0 { read (GR0_Reg32::IDR0) }, idr1 { read (GR0_Reg32::IDR1) }, idr2 { read (GR0_Reg32::IDR2) }, page_size { idr1 & BIT (31) ? BIT (16) : BIT (12) }, board (brd)
{
    // Determine total SMMU size
    auto const smmu_pnum { BIT ((idr1 >> 28 & BIT_RANGE (2, 0)) + 1) };
    auto const smmu_size { page_size * smmu_pnum * 2 };

    // Map and reserve remaining MMIO pages
    alloc_mmio (phys + PAGE_SIZE (0), smmu_size - PAGE_SIZE (0), Memattr::dev(), true);
    mmio_base_gr1 = mmio + page_size;
    mmio_base_ctx = mmio + page_size * smmu_pnum;

    // Determine SMMU capabilities
    mode      = feat_sms() ? Mode::STREAM_MATCHING : Mode::STREAM_INDEXING;
    sidx_bits = feat_exids() ? 16 : idr0 >> 9 & BIT_RANGE (3, 0);
    ias       = BIT_RANGE (3, 0) & idr2;
    oas       = BIT_RANGE (3, 0) & idr2 >> 4;

    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt::noncoherent |= !feat_cttw();

    // Reduce the number of domain IDs to what this SMMU supports
    Sdid::allocator.reduce (BIT (feat_vmid16() ? 16 : 8));

    auto const idr7 { read (GR0_Reg32::IDR7) };

    trace (TRACE_SMMU, "SMMU: %#010lx %#x r%up%u S1:%u S2:%u N:%u C:%u SMG:%u CTX:%u SID:%u-bit Mode:%u",
           phys, smmu_size, idr7 >> 4 & BIT_RANGE (3, 0), idr7 & BIT_RANGE (3, 0),
           feat_s1ts(), feat_s2ts(), feat_nts(), feat_cttw(),
           num_smg(), num_ctx(), sidx_bits, std::to_underlying (mode));
}

Smmu_v2 *Smmu_v2::create (Board::Smmu_v2 const &board)
{
    // Allocate SMMU
    auto const smmu { new Smmu_v2 { board } };

    // Add to SMMU list only if all structure allocations succeeded
    if (smmu) [[likely]] {
        smmu->insert (list);
        return smmu;
    }

    return nullptr;
}

bool Smmu_v2::init()
{
    // Configure global fault interrupts
    for (unsigned i { 0 }; i < sizeof (board.glb) / sizeof (*board.glb); i++)
        if (board.glb[i].flg)
            Gicd::conf_std (Intid::from_spi (board.glb[i].spi), Cpu::id, board.glb[i].flg & BIT_RANGE (3, 2));

    // Configure context fault interrupts
    for (unsigned i { 0 }; i < sizeof (board.ctx) / sizeof (*board.ctx); i++)
        if (board.ctx[i].flg)
            Gicd::conf_std (Intid::from_spi (board.ctx[i].spi), Cpu::id, board.ctx[i].flg & BIT_RANGE (3, 2));

    // Disable all SMGs
    for (uint8_t smg { 0 }; smg < num_smg(); smg++) {
        write (smg, GR0_Arr32::SM, 0);
        write (smg, GR0_Arr32::S2C, BIT (17));
    }

    // Disable all CTXs
    for (uint8_t ctx { 0 }; ctx < num_ctx(); ctx++)
        write (ctx, GR1_Arr32::CBA, BIT (17));

    write (GR0_Reg32::CR0, feat_vmid16() * BIT (31) | BIT (21) | BIT_RANGE (12, 10) | BIT_RANGE (5, 4) | feat_exids() * BIT (3) | BIT_RANGE (2, 1));

    return true;
}

Status Smmu_v2::assign_dev (Dc_state const *dc, Space_dma *o, Space_dma *n, uintptr_t &sbw)
{
    assert (o || n);

    auto const sid { static_cast<uint16_t>(dc->sid) };
    auto const msk { static_cast<uint16_t>(dc->sid >> 16) };

    auto       smg { dc->smg };
    auto const ctx { dc->ctx };

    // When using stream indexing, the maximum SID size is 7 bits and selects the SMG directly
    if (mode == Mode::STREAM_INDEXING)
        smg = static_cast<uint8_t>(sid);

    // Source SID must be within range supported by the SMMU
    if ((sid | msk) >= BIT (sidx_bits)) [[unlikely]]
        return Status::BAD_DEV;

    // SMG/CTX indices encoded in the Device Context must be within the SMMU's implemented range
    if (smg >= num_smg() || ctx >= num_ctx()) [[unlikely]]
        return Status::BAD_PAR;

    // Accumulates invalidation timeouts
    bool inv_timeout { false };

    // Acquire exclusive ownership of SMMU config registers
    Lock_guard <Spinlock> guard { cfg_lock };

    // Determine current CTX state
    auto const cba  { read (ctx, GR1_Arr32::CBA) };
    auto const cba2 { read (ctx, GR1_Arr32::CBA2) };
    auto const vmid { feat_vmid16() ? cba2 >> 16 & BIT_RANGE (15, 0) : cba & BIT_RANGE (7, 0) };
    auto used { (cba & BIT_RANGE (17, 16)) == 0 };

    // Report incorrect o
    if (o) {
        // Detach: CTX must be used and must match o
        if (!used || vmid != o->get_sdid()) [[unlikely]]
            return Status::BAD_PAR;
    } else {
        // Attach: CTX must be unused or must match n
        if (used && vmid != n->get_sdid()) [[unlikely]]
            return Status::BAD_PAR;
    }

    // Phase 1: Detach
    if (o) {

        // Unconfigure SMG always
        write (smg, GR0_Arr32::SM, 0);
        write (smg, GR0_Arr32::S2C, BIT (17));

        // Unconfigure CTX only when last SMG detaches
        if (!(used = ctx_used (ctx))) {

            // Disable => Fault
            write (ctx, GR1_Arr32::CBA, BIT (17));

            // Invalidate TLB
            inv_timeout |= !invalidate_tlb (o->get_sdid());

            // Release reference for o
            o->ref_dec();
        }
    }

    // Determine input address size and PTAB levels
    auto const isz { min (Dpt::pas (ias), Dpt::ibits) };
    auto const ptl { Dpt::lev (isz) };

    // Phase 2: Attach
    if (n) {

        // Configure CTX only when first SMG attaches
        if (!used) {

            // Unable to lookup/allocate PTAB root
            auto const ptr { n->get_root (ptl) };
            if (!ptr) [[unlikely]]
                return Status::MEM_OBJ;

            // Acquire reference for n
            if (!n->try_inc()) [[unlikely]]
                return Status::ABORTED;

            // Configure and enable CTX
            write (ctx, GR1_Arr32::CBA2,  feat_vmid16() * (n->get_sdid() << 16) | BIT (0));
            write (ctx, GR1_Arr32::CBA,   !feat_vmid16() * n->get_sdid());
            write (ctx, Ctx_Arr32::TCR,   oas << 16 | TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | (ptl - 2) << 6 | (64 - isz));
            write (ctx, Ctx_Arr64::TTBR0, Kmem::ptr_to_phys (ptr));
            write (ctx, Ctx_Arr32::SCTLR, BIT_RANGE (6, 5) | BIT (0));
        }

        // Configure SMG always
        write (smg, GR0_Arr32::S2C, BIT (27) | feat_exids() * BIT (10) | ctx);
        write (smg, GR0_Arr32::SM, !feat_exids() * BIT (31) | msk << 16 | sid);

        trace (TRACE_SMMU, "SMMU: %#010lx Assigned SID %#06x to Domain %#06x", phys, sid, n->get_sdid());
    }

    if (inv_timeout) [[unlikely]] {
        trace (TRACE_ERROR, "SMMU: %#010lx Invalidation Timeout (Faulty HW?)", phys);
        return Status::TIMEOUT;
    }

    // Effective selector bit width for this SMMU
    sbw = min (Dpt::ibits, isz) - PAGE_BITS;

    return Status::SUCCESS;
}

void Smmu_v2::fault (unsigned)
{
    auto const gfsr { read (GR0_Reg32::GFSR) };

    if (gfsr & BIT_RANGE (8, 0)) {

        auto const syn { read (GR0_Reg32::GFSYNR0) };

        trace (TRACE_SMMU, "SMMU: GLB Fault (M:%u UUT:%u P:%u E:%u CA:%u UCI:%u UCB:%u SMC:%u US:%u IC:%u) at %#010lx (%c%c%c) SID:%#x",
               !!(gfsr & BIT (31)), !!(gfsr & BIT (8)), !!(gfsr & BIT (7)), !!(gfsr & BIT (6)), !!(gfsr & BIT (5)),
               !!(gfsr & BIT (4)),  !!(gfsr & BIT (3)), !!(gfsr & BIT (2)), !!(gfsr & BIT (1)), !!(gfsr & BIT (0)),
               read (GR0_Reg64::GFAR),
               syn & BIT (3) ? 'I' : 'D',       // Instruction / Data
               syn & BIT (2) ? 'P' : 'U',       // Privileged / Unprivileged
               syn & BIT (1) ? 'W' : 'R',       // Write / Read
               read (GR0_Reg32::GFSYNR1) & BIT_RANGE (15, 0));

        write (GR0_Reg32::GFSR, gfsr);
    }

    for (unsigned ctx { 0 }; ctx < num_ctx(); ctx++) {

        auto const fsr { read (ctx, Ctx_Arr32::FSR) };

        if (fsr & BIT_RANGE (8, 1)) {

            auto const syn { read (ctx, Ctx_Arr32::FSYNR0) };

            trace (TRACE_SMMU, "SMMU: C%02u Fault (M:%u SS:%u UUT:%u AS:%u LK:%u MC:%u E:%u P:%u A:%u T:%u) at %#010lx (%c%c%c) LVL:%u",
                   ctx, !!(fsr & BIT (31)), !!(fsr & BIT (30)),
                   !!(fsr & BIT (8)), !!(fsr & BIT (7)), !!(fsr & BIT (6)), !!(fsr & BIT (5)),
                   !!(fsr & BIT (4)), !!(fsr & BIT (3)), !!(fsr & BIT (2)), !!(fsr & BIT (1)),
                   read (ctx, Ctx_Arr64::FAR),
                   syn & BIT (6) ? 'I' : 'D',   // Instruction / Data
                   syn & BIT (5) ? 'P' : 'U',   // Privileged / Unprivileged
                   syn & BIT (4) ? 'W' : 'R',   // Write / Read
                   syn & BIT_RANGE (1, 0));

            write (ctx, Ctx_Arr32::FSR, fsr);
        }
    }
}
