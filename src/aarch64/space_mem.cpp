/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "pd_kern.hpp"

void Space_mem::sync (Space::Index si)
{
    switch (si) {
        case Space::Index::CPU_HST: cpu_hst.invalidate (id_hst); break;
        case Space::Index::CPU_GST: cpu_gst.invalidate (id_gst); break;
        case Space::Index::DMA_HST: dma_hst.invalidate (id_hst); break;
        case Space::Index::DMA_GST: dma_gst.invalidate (id_gst); break;
    }
}

Status Space_mem::update (uint64 v, uint64 p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh, Space::Index si)
{
    switch (si) {
        case Space::Index::CPU_HST: return cpu_hst.update (v, p, o, pm, ca, sh);
        case Space::Index::CPU_GST: return cpu_gst.update (v, p, o, pm, ca, sh);
        case Space::Index::DMA_HST: return dma_hst.update (v, p, o, pm, ca, sh, Smmu::nc);
        case Space::Index::DMA_GST: return dma_gst.update (v, p, o, pm, ca, sh, Smmu::nc);
    }

    return Status::BAD_PAR;
}

Status Space_mem::delegate (Space_mem const *mem, unsigned long src, unsigned long dst, unsigned ord, unsigned pmm, Space::Index si, Memattr::Cacheability ca, Memattr::Shareability sh)
{
    auto const s_end = src + BITN (ord), d_end = dst + BITN (ord);

    if (EXPECT_FALSE (s_end > num || d_end > num))
        return Status::BAD_PAR;

    unsigned o;

    Status sts = Status::SUCCESS;

    for (auto s_sel = src, d_sel = dst; s_sel < s_end; s_sel += BITN (o), d_sel += BITN (o)) {

        uintptr_t s = s_sel << PAGE_BITS;
        uintptr_t d = d_sel << PAGE_BITS;
        Hpt::OAddr p;
        Memattr::Cacheability src_ca;
        Memattr::Shareability src_sh;

        auto pm = Paging::Permissions (mem->lookup (s, p, o, src_ca, src_sh) & (Paging::K | Paging::U | pmm));

        // Kernel memory cannot be delegated
        if (pm & Paging::K)
            pm = Paging::NONE;

        // Memory attributes are inherited for virt/virt delegations
        if (mem != &Pd_kern::nova()) {
            ca = src_ca;
            sh = src_sh;
        }

        o = min (o, ord);

        s &= ~Hpt::offs_mask (o);
        d &= ~Hpt::offs_mask (o);
        p &= ~Hpt::offs_mask (o);

        if ((sts = update (d, p, o, pm, ca, sh, si)) != Status::SUCCESS)
            break;
    }

    sync (si);

    Buddy::free_wait();

    return sts;
}
