/*
 * System Memory Management Unit (SMMUv2)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "hpt.hpp"
#include "interrupt.hpp"
#include "lock_guard.hpp"
#include "lowlevel.hpp"
#include "pd.hpp"
#include "smmu.hpp"
#include "stdio.hpp"

Spinlock Smmu::lock;
mword Smmu::gr0_base, Smmu::gr1_base, Smmu::ctx_base;
unsigned Smmu::page_size, Smmu::pa_size, Smmu::sidb, Smmu::smrg, Smmu::ctxb;
bool Smmu::nc;

/*
 * XXX: Missing pieces
 *
 * Extended Stream Matching
 * Stream Indexing
 * Compressed Stream Indexing
 */
void Smmu::init()
{
    if (!SMMU_SIZE)
        return;

    Hptp::master.update (DEV_GLOBL_SMMU, SMMU_BASE, 0,
                         Paging::Permissions (Paging::R | Paging::W | Paging::G),
                         Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

    gr0_base = DEV_GLOBL_SMMU;

    auto idr0 = read (GR0_Register32::IDR0);
    auto idr1 = read (GR0_Register32::IDR1);
    auto idr2 = read (GR0_Register32::IDR2);
    auto idr7 = read (GR0_Register32::IDR7);

    unsigned page_num = BIT ((idr1 >> 28 & 0x7) + 1);
    page_size = idr1 >> 31 & 0x1 ? 16 * PAGE_SIZE : PAGE_SIZE;
    pa_size = idr2 >> 4 & BIT_RANGE (2, 0);

    mword smmu_size = page_num * page_size * 2;

    gr1_base = gr0_base + page_size;
    ctx_base = gr0_base + page_size * page_num;

    Hptp::master.update (DEV_GLOBL_SMMU, SMMU_BASE, static_cast<unsigned>(bit_scan_reverse (smmu_size)) - PAGE_BITS,
                         Paging::Permissions (Paging::R | Paging::W | Paging::G),
                         Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

    trace (TRACE_IOMMU, "SMMU: %#010x %#lx r%up%u S1:%u S2:%u NTS:%u SMS:%u CTTW:%u BTM:%u PA:%u",
           SMMU_BASE, smmu_size, idr7 >> 4 & BIT_RANGE (3, 0), idr7 & BIT_RANGE (3, 0),
           !!(idr0 & BIT (30)), !!(idr0 & BIT (29)), !!(idr0 & BIT (28)), !!(idr0 & BIT (27)),
           !!(idr0 & BIT (14)), !!(idr0 & BIT (13)), pa_size);

    nc = !(idr0 & BIT (14));
    sidb = idr0 >>  9 & BIT_RANGE (3, 0);
    smrg = idr0       & BIT_RANGE (7, 0);
    ctxb = idr1       & BIT_RANGE (7, 0);

    // Stream Matching
    if (idr0 & BIT (27)) {

        trace (TRACE_IOMMU, "SMMU: Stream matching with %u CTXs, %u SMRs, %u-bit SIDs, PS:%#x NC:%u", ctxb, smrg, sidb, page_size, nc);

        for (unsigned i = 0; i < smrg; i++) {
            write (i, GR0_Array32::SMR, 0);
            write (i, GR0_Array32::S2CR, BIT (17));
        }
    }

    Interrupt::conf_spi (SMMU_SPI, 0, false, SMMU_FLG & 0x3, false);

    write (GR0_Register32::CR0, BIT (21) | BIT (10) | BIT_RANGE (5, 4) | BIT_RANGE (2, 1));
}

bool Smmu::configure (unsigned sid, unsigned smg, unsigned ctx, Pd *pd, Space::Index si)
{
    if (sid >= BIT (sidb) || smg >= smrg || ctx >= ctxb || !Space::is_dma (si))
        return false;

    auto vmid = Space::is_gst (si) ? pd->vmid_gst() : pd->vmid_hst();
    auto ttbr = Space::is_gst (si) ? pd->ptab_gst() : pd->ptab_hst();

    trace (TRACE_IOMMU, "SMMU: %s: sid=%#x smg=%u ctx=%u pd=%p vmid=%lu ttbr=%#llx", __func__, sid, smg, ctx, static_cast<void *>(pd), vmid.vmid(), ttbr);

    {   Lock_guard <Spinlock> guard (lock);

        write (smg, GR0_Array32::SMR,   BIT (31) | sid);
        write (smg, GR0_Array32::S2CR,  ctx);

        write (ctx, GR1_Array32::CBAR,  vmid.vmid() & BIT_RANGE (7, 0));
        write (ctx, GR1_Array32::CBA2R, BIT (0));

        write (ctx, Ctx_Array64::TTBR0, ttbr);
        write (ctx, Ctx_Array32::TCR,   pa_size << 16 | TCR_A64_TG0_4K | TCR_ALL_SH0_IS | TCR_ALL_ORGN0_WB_WA | TCR_ALL_IRGN0_WB_WA | VTCR_SL0_L1 | VTCR_T0SZ (IPA_BITS));
        write (ctx, Ctx_Array32::SCTLR, BIT_RANGE (6, 5) | BIT (0));
    }

    return true;
}

void Smmu::flush (Vmid vmid)
{
    write (GR0_Register32::TLBIVMID, vmid.vmid() & BIT_RANGE (15, 0));

    {   Lock_guard <Spinlock> guard (lock);

        write (GR0_Register32::TLBGSYNC, 0);

        while (read (GR0_Register32::TLBGSTATUS) & BIT (0))
            pause();
    }
}

void Smmu::interrupt()
{
    uint32 gfsr, fsr;

    if ((gfsr = read (GR0_Register32::GFSR)) & BIT_RANGE (8, 0)) {

        auto syn = read (GR0_Register32::GFSYNR0);

        trace (TRACE_IOMMU, "SMMU: GLB Fault (M:%u UUT:%u P:%u E:%u CA:%u UCI:%u UCB:%u SMC:%u US:%u IC:%u) at %#010llx (%c%c%c) SID:%#x",
               !!(gfsr & BIT (31)), !!(gfsr & BIT (8)), !!(gfsr & BIT (7)), !!(gfsr & BIT (6)), !!(gfsr & BIT (5)),
               !!(gfsr & BIT (4)),  !!(gfsr & BIT (3)), !!(gfsr & BIT (2)), !!(gfsr & BIT (1)), !!(gfsr & BIT (0)),
               read (GR0_Register64::GFAR),
               (syn & BIT (3)) ? 'I' : 'D',         // Instruction / Data
               (syn & BIT (2)) ? 'P' : 'U',         // Privileged / Unprivileged
               (syn & BIT (1)) ? 'W' : 'R',         // Write / Read
               read (GR0_Register32::GFSYNR1) & BIT_RANGE (15, 0));

        write (GR0_Register32::GFSR, gfsr);
    }

    for (unsigned ctx = 0; ctx < ctxb; ctx++) {

        if ((fsr = read (ctx, Ctx_Array32::FSR)) & BIT_RANGE (8, 1)) {

            auto syn = read (ctx, Ctx_Array32::FSYNR0);

            trace (TRACE_IOMMU, "SMMU: C%02u Fault (M:%u SS:%u UUT:%u AS:%u LK:%u MC:%u E:%u P:%u A:%u T:%u) at %#010llx (%c%c%c) LVL:%u",
                   ctx, !!(fsr & BIT (31)), !!(fsr & BIT (30)),
                   !!(fsr & BIT (8)), !!(fsr & BIT (7)), !!(fsr & BIT (6)), !!(fsr & BIT (5)),
                   !!(fsr & BIT (4)), !!(fsr & BIT (3)), !!(fsr & BIT (2)), !!(fsr & BIT (1)),
                   read (ctx, Ctx_Array64::FAR),
                   (syn & BIT (6)) ? 'I' : 'D',     // Instruction / Data
                   (syn & BIT (5)) ? 'P' : 'U',     // Privileged / Unprivileged
                   (syn & BIT (4)) ? 'W' : 'R',     // Write / Read
                   (syn & BIT_RANGE (1, 0)));

            write (ctx, Ctx_Array32::FSR, fsr);
        }
    }
}
