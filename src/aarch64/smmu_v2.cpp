/*
 * System Memory Management Unit (Arm SMMUv2)
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

#include "interrupt.hpp"
#include "lock_guard.hpp"
#include "smmu_v2.hpp"
#include "space_dma.hpp"
#include "wait.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu_v2::cache { sizeof (Smmu_v2), alignof (Smmu_v2) };

Smmu_v2::Smmu_v2 (Board::Smmu_v2 const &brd) : Smmu { brd.mmio, PAGE_SIZE (0) }, idr0 { read (GR0_Reg32::IDR0) }, idr1 { read (GR0_Reg32::IDR1) }, idr2 { read (GR0_Reg32::IDR2) }, page_size { idr1 & BIT (31) ? BIT (16) : BIT (12) }, board (brd)
{
    // Determine total SMMU size
    auto const smmu_pnum { BIT ((idr1 >> 28 & BIT_RANGE (2, 0)) + 1) };
    auto const smmu_size { page_size * smmu_pnum * 2 };

    // Map remaining MMIO pages
    alloc_mmio (phys + PAGE_SIZE (0), smmu_size - PAGE_SIZE (0));
    mmio_base_gr1 = mmio + page_size;
    mmio_base_ctx = mmio + page_size * smmu_pnum;

    // Determine SMMU capabilities
    mode      = idr0 & BIT (27) ? Mode::STREAM_MATCHING : Mode::STREAM_INDEXING;
    sidx_bits = idr0 & BIT (8) ? 16 : idr0 >> 9 & BIT_RANGE (3, 0);
    ias       = BIT_RANGE (3, 0) & idr2;
    oas       = BIT_RANGE (3, 0) & idr2 >> 4;

    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt::noncoherent |= !(idr0 & BIT (14));

    // Allocate configuration table
    config = new Config;

    auto const idr7 { read (GR0_Reg32::IDR7) };

    trace (TRACE_SMMU, "SMMU: %#010lx %#x r%up%u S1:%u S2:%u N:%u C:%u SMG:%u CTX:%u SID:%u-bit Mode:%u",
           phys, smmu_size, idr7 >> 4 & BIT_RANGE (3, 0), idr7 & BIT_RANGE (3, 0),
           !!(idr0 & BIT (30)), !!(idr0 & BIT (29)), !!(idr0 & BIT (28)), !!(idr0 & BIT (14)),
           num_smg(), num_ctx(), sidx_bits, std::to_underlying (mode));
}

bool Smmu_v2::init()
{
    // Configure global fault interrupts
    for (unsigned i { 0 }; i < sizeof (board.glb) / sizeof (*board.glb); i++)
        if (board.glb[i].flg)
            Interrupt::configure (Intid::from_spi (board.glb[i].spi), board.glb[i].flg & BIT_RANGE (3, 2), false, false, Cpu::id);

    // Configure context fault interrupts
    for (unsigned i { 0 }; i < sizeof (board.ctx) / sizeof (*board.ctx); i++)
        if (board.ctx[i].flg)
            Interrupt::configure (Intid::from_spi (board.ctx[i].spi), board.ctx[i].flg & BIT_RANGE (3, 2), false, false, Cpu::id);

    // Configure CTXs
    for (uint8_t ctx { 0 }; ctx < num_ctx(); ctx++)
        write (ctx, GR1_Arr32::CBAR, BIT (17));       // Generate "invalid context" fault

    // Configure SMGs
    for (uint8_t smg { 0 }; smg < num_smg(); smg++)
        if (!conf_smg (smg))
            write (smg, GR0_Arr32::S2CR, BIT (17));   // Generate "invalid context" fault

    write (GR0_Reg32::CR0, BIT (21) | BIT_RANGE (12, 10) | BIT_RANGE (5, 4) | BIT_RANGE (2, 1));

    return true;
}

bool Smmu_v2::conf_smg (uint8_t smg)
{
    // Obtain SMG configuration
    auto const dma { config->entry[smg].dma };
    auto const sid { config->entry[smg].sid };
    auto const msk { config->entry[smg].msk };
    auto const ctx { config->entry[smg].ctx };

    if (!dma)
        return false;

    auto const sdid { dma->get_sdid() };

    // Disable CTX during configuration
    write (ctx, Ctx_Arr32::SCTLR, 0);

    // Invalidate stale TLB entries for SDID
    tlb_invalidate (sdid);

    // Configure CTX as VA64 stage-2
    write (ctx, GR1_Arr32::CBA2R, BIT (0));
    write (ctx, GR1_Arr32::CBAR,  sdid & BIT_RANGE (7, 0));

    // Determine input size and number of levels
    auto const isz { Dpt::pas (ias) };
    auto const lev { Dpt::lev (isz) };

    // Configure and enable CTX
    write (ctx, Ctx_Arr32::TCR,   oas << 16 | TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | (lev - 2) << 6 | (64 - isz));
    write (ctx, Ctx_Arr64::TTBR0, Kmem::ptr_to_phys (dma->get_ptab (lev - 1)));
    write (ctx, Ctx_Arr32::SCTLR, BIT_RANGE (6, 5) | BIT (0));

    // Disable SMG during configuration
    write (smg, GR0_Arr32::SMR, 0);

    // Configure and enable SMG
    write (smg, GR0_Arr32::S2CR, BIT (27) | ctx);
    write (smg, GR0_Arr32::SMR,  BIT (31) | msk << 16 | sid);

    return true;
}

Status Smmu_v2::assign_dev (Space_dma *dma, uintptr_t dad)
{
    auto const sid { static_cast<uint16_t>(dad) };
    auto const msk { static_cast<uint16_t>(dad >> 16) };
    auto       smg { static_cast<uint8_t> (dad >> 32) };
    auto const ctx { static_cast<uint8_t> (dad >> 40) };

    // When using stream indexing, the maximum SID size is 7 bits and selects the SMG directly
    if (mode == Mode::STREAM_INDEXING)
        smg = static_cast<uint8_t>(sid);

    if (!config || (sid | msk) >= BIT (sidx_bits) || smg >= num_smg() || ctx >= num_ctx())
        return Status::BAD_PAR;

    trace (TRACE_SMMU, "SMMU: SID:%#06x MSK:%#06x SMG:%#04x CTX:%#04x assigned to Domain %u", sid, msk, smg, ctx, static_cast<unsigned>(dma->get_sdid()));

    Lock_guard <Spinlock> guard { cfg_lock };

    // Remember SMG configuration for suspend/resume
    config->entry[smg].dma = dma;
    config->entry[smg].sid = sid;
    config->entry[smg].msk = msk;
    config->entry[smg].ctx = ctx;

    return conf_smg (smg) ? Status::SUCCESS : Status::BAD_PAR;
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

/*
 * TLB Invalidate by IPA
 */
bool Smmu_v2::tlb_invalidate (unsigned ctx, uint64_t ipa)
{
    // Post TLB maintenance operation
    write (ctx, Ctx_Arr64::TLBIIPAS2, ipa >> 12);

    // Ensure completion
    return tlb_sync_ctx (ctx);
}

/*
 * TLB Invalidate by VMID
 */
bool Smmu_v2::tlb_invalidate (Sdid vmid)
{
    // Post TLB maintenance operation
    write (GR0_Reg32::TLBIVMID, vmid & BIT_RANGE (15, 0));

    // Ensure completion
    return tlb_sync_glb();
}

/*
 * Ensure completion of one or more posted TLB invalidate operations
 * accepted in the specified translation context bank only.
 */
bool Smmu_v2::tlb_sync_ctx (unsigned ctx)
{
    // Lock ensures TLB sync issued by this CPU is not starved by more syncs issued by other CPUs
    Lock_guard <Spinlock> guard { inv_lock };

    write (ctx, Ctx_Arr32::TLBSYNC, 0);

    // Wait until hardware clears the active bit
    return Wait::until (timeout, [&] { return (read (ctx, Ctx_Arr32::TLBSTATUS) & BIT (0)) == 0; });
}

/*
 * Ensure completion of one or more posted TLB invalidate operations
 * accepted in the global address space or in any translation context bank.
 */
bool Smmu_v2::tlb_sync_glb()
{
    // Lock ensures TLB sync issued by this CPU is not starved by more syncs issued by other CPUs
    Lock_guard <Spinlock> guard { inv_lock };

    write (GR0_Reg32::TLBGSYNC, 0);

    // Wait until hardware clears the active bit
    return Wait::until (timeout, [&] { return (read (GR0_Reg32::TLBGSTATUS) & BIT (0)) == 0; });
}
