/*
 * System Memory Management Unit (ARM SMMUv2)
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "interrupt.hpp"
#include "lock_guard.hpp"
#include "lowlevel.hpp"
#include "pd_kern.hpp"
#include "smmu.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache  Smmu::cache (sizeof (Smmu), 8);

Smmu::Smmu (Board::Smmu const &cfg) : List (list), config (cfg)
{
    Hptp::master.update (mmap, config.mmio, 0,
                         Paging::Permissions (Paging::G | Paging::W | Paging::R),
                         Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

    mmio_base_gr0 = mmap;

    auto idr0 = read (GR0_Register32::IDR0);
    auto idr1 = read (GR0_Register32::IDR1);
    auto idr2 = read (GR0_Register32::IDR2);
    auto idr7 = read (GR0_Register32::IDR7);
    auto pnum = BIT ((idr1 >> 28 & BIT_RANGE (2, 0)) + 1);

    mode          = idr0 & BIT (27) ? Mode::STREAM_MATCHING : Mode::STREAM_INDEXING;
    page_size     = idr1 & BIT (31) ? 16 * PAGE_SIZE : PAGE_SIZE;
    phys_bits     = idr2 >> 4 & BIT_RANGE (2, 0);
    sidx_bits     = idr0 >> 9 & BIT_RANGE (3, 0);
    mmio_base_gr1 = mmio_base_gr0 + page_size;
    mmio_base_ctx = mmio_base_gr0 + page_size * pnum;

    mword smmu_size = pnum * page_size * 2;

    Hptp::master.update (mmap, config.mmio, static_cast<unsigned>(bit_scan_reverse (smmu_size)) - PAGE_BITS,
                         Paging::Permissions (Paging::G | Paging::W | Paging::R),
                         Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

    nc |= !(idr0 & BIT (14));

    // FIXME: Needs per-SMMU HIP entries
    num_smg = idr0 & BIT_RANGE (7, 0);
    num_ctx = idr1 & BIT_RANGE (7, 0);

    trace (TRACE_SMMU, "SMMU: %#010llx %#lx r%up%u S1:%u S2:%u SMG:%u CTX:%u SID:%u-bit Mode:%u",
           config.mmio, smmu_size, idr7 >> 4 & BIT_RANGE (3, 0), idr7 & BIT_RANGE (3, 0),
           !!(idr0 & BIT (30)), !!(idr0 & BIT (29)), num_smg, num_ctx, sidx_bits, static_cast<unsigned>(mode));

    // Reserve MMIO region
    Pd_kern::remove_user_mem (config.mmio, smmu_size);

    // Advance memory map pointer
    mmap += smmu_size;
}

void Smmu::init()
{
    // Configure all SMGs to generate "invalid context" fault
    for (unsigned i = 0; i < num_smg; i++)
        write (i, GR0_Array32::S2CR, BIT (17));

    // Configure all CTXs to generate "invalid context" fault
    for (unsigned i = 0; i < num_ctx; i++)
        write (i, GR1_Array32::CBAR, BIT (17));

    // Configure global fault interrupts
    for (unsigned i = 0; i < sizeof (config.glb) / sizeof (*config.glb); i++)
        if (config.glb[i].flg)
            Interrupt::conf_spi (config.glb[i].spi, Cpu::id, false, config.glb[i].flg & BIT_RANGE (3, 2));

    // Configure context fault interrupts
    for (unsigned i = 0; i < sizeof (config.ctx) / sizeof (*config.ctx); i++)
        if (config.ctx[i].flg)
            Interrupt::conf_spi (config.ctx[i].spi, Cpu::id, false, config.ctx[i].flg & BIT_RANGE (3, 2));

    write (GR0_Register32::CR0, BIT (21) | BIT (10) | BIT_RANGE (5, 4) | BIT_RANGE (2, 1));
}

bool Smmu::configure (Pd *pd, Space::Index si, mword dev)
{
    auto ctx = uint8  (dev >> 24);
    auto smg = uint8  (dev >> 16);
    auto sid = uint16 (dev);

    // When using stream indexing, the maximum SID size is 7 bits and selects the SMG directly
    if (mode == Mode::STREAM_INDEXING)
        smg = uint8 (sid);

    if (sid >= BIT (sidx_bits) || smg >= num_smg || ctx >= num_ctx || !Space::is_dma (si))
        return false;

    auto vmid = Space::is_gst (si) ? pd->vmid_gst() : pd->vmid_hst();
    auto ttbr = Space::is_gst (si) ? pd->ptab_gst() : pd->ptab_hst();

    trace (TRACE_SMMU, "SMMU: %s: SID:%#06x SMG:%02u CTX:%02u assigned to PD:%p", __func__, sid, smg, ctx, static_cast<void *>(pd));

    {   Lock_guard <Spinlock> guard (lock);

        // First configure the CTX
        write (ctx, Ctx_Array64::TTBR0, ttbr);
        write (ctx, Ctx_Array32::TCR,   phys_bits << 16 | TCR_A64_TG0_4K | TCR_ALL_SH0_INNER | TCR_ALL_ORGN0_WB_RW | TCR_ALL_IRGN0_WB_RW | VTCR_SL0_L1 | VTCR_T0SZ (IPA_BITS));
        write (ctx, Ctx_Array32::SCTLR, BIT_RANGE (6, 5) | BIT (0));

        write (ctx, GR1_Array32::CBAR,  vmid & BIT_RANGE (7, 0));
        write (ctx, GR1_Array32::CBA2R, BIT (0));

        // Then configure the SMG and point it to the CTX
        write (smg, GR0_Array32::SMR,   BIT (31) | sid);
        write (smg, GR0_Array32::S2CR,  ctx);
    }

    return true;
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

void Smmu::flush (Vmid vmid)
{
    write (GR0_Register32::TLBIVMID, vmid & BIT_RANGE (15, 0));

    {   Lock_guard <Spinlock> guard (lock);

        write (GR0_Register32::TLBGSYNC, 0);

        while (read (GR0_Register32::TLBGSTATUS) & BIT (0))
            pause();
    }
}
