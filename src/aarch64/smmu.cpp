/*
 * System Memory Management Unit (ARM SMMUv2)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "hip.hpp"
#include "interrupt.hpp"
#include "lock_guard.hpp"
#include "lowlevel.hpp"
#include "pd_kern.hpp"
#include "smmu.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Smmu::cache (sizeof (Smmu), 8);

Smmu::Smmu (Board::Smmu const &brd) : List (list), board (brd)
{
    // Map first SMMU page
    Hptp::master.update (mmap, board.mmio, 0,
                         Paging::Permissions (Paging::G | Paging::W | Paging::R),
                         Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

    // This facilitates access to the GR0 register space only
    mmio_base_gr0 = mmap;

    auto idr0 = read (GR0_Register32::IDR0);
    auto idr1 = read (GR0_Register32::IDR1);
    auto idr2 = read (GR0_Register32::IDR2);
    auto idr7 = read (GR0_Register32::IDR7);

    // Determine SMMU capabilities
    page_size = idr1 & BIT (31) ? BIT (16) : BIT (12);
    phys_bits = idr2 >> 4 & BIT_RANGE (2, 0);
    sidx_bits = idr0 & BIT (8) ? 16 : idr0 >> 9 & BIT_RANGE (3, 0);
    num_smg   = idr0 & BIT_RANGE (7, 0);
    num_ctx   = idr1 & BIT_RANGE (7, 0);
    mode      = idr0 & BIT (27) ? Mode::STREAM_MATCHING : Mode::STREAM_INDEXING;

    // True if at least one SMMU is non-coherent
    nc |= !(idr0 & BIT (14));

    // Determine total size of the SMMU
    auto smmu_pnum = BIT ((idr1 >> 28 & BIT_RANGE (2, 0)) + 1);
    auto smmu_size = page_size * smmu_pnum * 2;

    // Map all SMMU pages
    Hptp::master.update (mmap, board.mmio, static_cast<unsigned>(bit_scan_reverse (smmu_size)) - PAGE_BITS,
                         Paging::Permissions (Paging::G | Paging::W | Paging::R),
                         Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

    // This facilitates access to the GR1 and CTX register spaces
    mmio_base_gr1 = mmio_base_gr0 + page_size;
    mmio_base_ctx = mmio_base_gr0 + page_size * smmu_pnum;

    // Allocate configuration table
    config = new Config;

    trace (TRACE_SMMU, "SMMU: %#010llx %#x r%up%u NC:%u S1:%u S2:%u NTS:%u SMG:%u CTX:%u SID:%u-bit Mode:%u",
           board.mmio, smmu_size, idr7 >> 4 & BIT_RANGE (3, 0), idr7 & BIT_RANGE (3, 0), nc,
           !!(idr0 & BIT (30)), !!(idr0 & BIT (29)), !!(idr0 & BIT (28)), num_smg, num_ctx, sidx_bits, static_cast<unsigned>(mode));

    // Reserve MMIO region
    Pd_kern::remove_user_mem (board.mmio, smmu_size);

    // Advance memory map pointer
    mmap += smmu_size;

    Hip::set_feature (Hip_arch::Feature::SMMU);
}

void Smmu::init()
{
    // Configure global fault interrupts
    for (unsigned i = 0; i < sizeof (board.glb) / sizeof (*board.glb); i++)
        if (board.glb[i].flg)
            Interrupt::conf_spi (board.glb[i].spi, false, board.glb[i].flg & BIT_RANGE (3, 2), Cpu::id);

    // Configure context fault interrupts
    for (unsigned i = 0; i < sizeof (board.ctx) / sizeof (*board.ctx); i++)
        if (board.ctx[i].flg)
            Interrupt::conf_spi (board.ctx[i].spi, false, board.ctx[i].flg & BIT_RANGE (3, 2), Cpu::id);

    // Configure CTXs
    for (uint8 ctx = 0; ctx < num_ctx; ctx++)
        write (ctx, GR1_Array32::CBAR, BIT (17));       // Generate "invalid context" fault

    // Configure SMGs
    for (uint8 smg = 0; smg < num_smg; smg++)
        if (!conf_smg (smg))
            write (smg, GR0_Array32::S2CR, BIT (17));   // Generate "invalid context" fault

    write (GR0_Register32::CR0, BIT (21) | BIT (10) | BIT_RANGE (5, 4) | BIT_RANGE (2, 1));
}

bool Smmu::conf_smg (uint8 smg)
{
    // Obtain SMG configuration
    auto pd  = config->entry[smg].pd;
    auto si  = config->entry[smg].si;
    auto sid = config->entry[smg].sid;
    auto msk = config->entry[smg].msk;
    auto ctx = config->entry[smg].ctx;

    if (!pd)
        return false;

    auto vmid = Space::is_gst (si) ? pd->vmid_gst() : pd->vmid_hst();
    auto ptab = Space::is_gst (si) ? pd->rdma_gst() : pd->rdma_hst();

    if (!ptab)
        return false;

    // First configure the CTX
    write (ctx, Ctx_Array64::TTBR0, Kmem::ptr_to_phys (ptab));
    write (ctx, Ctx_Array32::TCR,   phys_bits << 16 | TCR_A64_TG0_4K | TCR_ALL_SH0_INNER | TCR_ALL_ORGN0_WB_RW | TCR_ALL_IRGN0_WB_RW | VTCR_SL0_L1 | VTCR_T0SZ (IPA_BITS));
    write (ctx, Ctx_Array32::SCTLR, BIT_RANGE (6, 5) | BIT (0));

    write (ctx, GR1_Array32::CBAR,  vmid & BIT_RANGE (7, 0));
    write (ctx, GR1_Array32::CBA2R, BIT (0));

    // Then configure the SMG and point it to the CTX
    write (smg, GR0_Array32::SMR,   BIT (31) | msk << 16 | sid);
    write (smg, GR0_Array32::S2CR,  ctx);

    return true;
}

bool Smmu::configure (Pd *pd, Space::Index si, uintptr_t dad)
{
    auto sid = static_cast<uint16>(dad);
    auto msk = static_cast<uint16>(dad >> 16);
    auto smg = static_cast<uint8> (dad >> 32);
    auto ctx = static_cast<uint8> (dad >> 40);

    // When using stream indexing, the maximum SID size is 7 bits and selects the SMG directly
    if (mode == Mode::STREAM_INDEXING)
        smg = static_cast<uint8>(sid);

    if (!config || (sid | msk) >= BIT (sidx_bits) || smg >= num_smg || ctx >= num_ctx || !Space::is_dma (si))
        return false;

    trace (TRACE_SMMU, "SMMU: SID:%#06x MSK:%#06x SMG:%#04x CTX:%#04x assigned to PD:%p", sid, msk, smg, ctx, static_cast<void *>(pd));

    Lock_guard <Spinlock> guard (lock);

    // Remember SMG configuration for suspend/resume
    config->entry[smg].pd  = pd;
    config->entry[smg].si  = si;
    config->entry[smg].sid = sid;
    config->entry[smg].msk = msk;
    config->entry[smg].ctx = ctx;

    return conf_smg (smg);
}

void Smmu::fault()
{
    uint32 gfsr, fsr;

    if ((gfsr = read (GR0_Register32::GFSR)) & BIT_RANGE (8, 0)) {

        auto syn = read (GR0_Register32::GFSYNR0);

        trace (TRACE_SMMU, "SMMU: GLB Fault (M:%u UUT:%u P:%u E:%u CA:%u UCI:%u UCB:%u SMC:%u US:%u IC:%u) at %#010llx (%c%c%c) SID:%#x",
               !!(gfsr & BIT (31)), !!(gfsr & BIT (8)), !!(gfsr & BIT (7)), !!(gfsr & BIT (6)), !!(gfsr & BIT (5)),
               !!(gfsr & BIT (4)),  !!(gfsr & BIT (3)), !!(gfsr & BIT (2)), !!(gfsr & BIT (1)), !!(gfsr & BIT (0)),
               read (GR0_Register64::GFAR),
               syn & BIT (3) ? 'I' : 'D',       // Instruction / Data
               syn & BIT (2) ? 'P' : 'U',       // Privileged / Unprivileged
               syn & BIT (1) ? 'W' : 'R',       // Write / Read
               read (GR0_Register32::GFSYNR1) & BIT_RANGE (15, 0));

        write (GR0_Register32::GFSR, gfsr);
    }

    for (unsigned ctx = 0; ctx < num_ctx; ctx++) {

        if ((fsr = read (ctx, Ctx_Array32::FSR)) & BIT_RANGE (8, 1)) {

            auto syn = read (ctx, Ctx_Array32::FSYNR0);

            trace (TRACE_SMMU, "SMMU: C%02u Fault (M:%u SS:%u UUT:%u AS:%u LK:%u MC:%u E:%u P:%u A:%u T:%u) at %#010llx (%c%c%c) LVL:%u",
                   ctx, !!(fsr & BIT (31)), !!(fsr & BIT (30)),
                   !!(fsr & BIT (8)), !!(fsr & BIT (7)), !!(fsr & BIT (6)), !!(fsr & BIT (5)),
                   !!(fsr & BIT (4)), !!(fsr & BIT (3)), !!(fsr & BIT (2)), !!(fsr & BIT (1)),
                   read (ctx, Ctx_Array64::FAR),
                   syn & BIT (6) ? 'I' : 'D',   // Instruction / Data
                   syn & BIT (5) ? 'P' : 'U',   // Privileged / Unprivileged
                   syn & BIT (4) ? 'W' : 'R',   // Write / Read
                   syn & BIT_RANGE (1, 0));

            write (ctx, Ctx_Array32::FSR, fsr);
        }
    }
}

void Smmu::invalidate (Vmid vmid)
{
    write (GR0_Register32::TLBIVMID, vmid & BIT_RANGE (15, 0));

    {   Lock_guard <Spinlock> guard (lock);

        write (GR0_Register32::TLBGSYNC, 0);

        while (read (GR0_Register32::TLBGSTATUS) & BIT (0))
            pause();
    }
}
