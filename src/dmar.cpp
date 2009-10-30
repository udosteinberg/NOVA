/*
 * DMA Remapping Unit (DMAR)
 *
 * Copyright (C) 2008-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "dmar.h"
#include "extern.h"
#include "lapic.h"
#include "pd.h"
#include "stdio.h"

Slab_cache Dmar::cache (sizeof (Dmar), 8);

Dmar *Dmar::list;

Rte *Rte::rtp = new Rte;

Dmar::Dmar (Paddr phys) : reg_base ((hwdev_addr -= PAGE_SIZE) | (phys & PAGE_MASK)), next (list)
{
    list = this;

    Pd::kern.Space_mem::insert (hwdev_addr, 0,
                                Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                                 Ptab::ATTR_GLOBAL      |
                                                 Ptab::ATTR_UNCACHEABLE |
                                                 Ptab::ATTR_WRITABLE),
                                phys & ~PAGE_MASK);

    uint64 cap  = read<uint64>(DMAR_CAP);
    frr_base  = static_cast<mword>(cap >> 20 & 0x3ff0) + reg_base;
    frr_count = static_cast<unsigned>(cap >> 40 & 0xff) + 1;

    unsigned dom = static_cast<mword>(cap) & 0x3;
    unsigned sps = static_cast<mword>(cap >> 34) & 0xf;
    unsigned gaw = static_cast<mword>(cap >>  8) & 0x1f;

    trace (0, "DMAR:%#lx FRR:%u DOM:%#x SPS:%#x GAW:%#x MSI:%#x",
           phys,
           frr_count,
           dom,
           sps,
           gaw,
           VEC_MSI_DMAR);

    write<uint32>(DMAR_FECTL,  0);
    write<uint32>(DMAR_FEDATA, VEC_MSI_DMAR);       // VEC
    write<uint32>(DMAR_FEADDR, 0xfee00000);         // MSI to CPU0

    assert (Rte::rtp);
    write<uint64>(DMAR_RTADDR, Buddy::ptr_to_phys (Rte::rtp));
    write<uint32>(DMAR_GCMD, GCMD_TE | GCMD_SRTP);
}

void Dmar::handle_faults()
{
    for (uint32 fsts; fsts = read<uint32>(DMAR_FSTS), fsts & 0xff;) {

        if (fsts & FSTS_PPF) {

            uint64 hi, lo;
            for (unsigned frr = fsts >> 8 & 0xff; read_frr (frr, hi, lo), hi & 1ull << 63; frr = (frr + 1) % frr_count) {

                trace (0, "DMAR:%p FRR:%u FR:%#x SID:%x:%02x:%x FI:%#010llx",
                       this,
                       frr,
                       static_cast<uint32>(hi >> 32) & 0xff,
                       static_cast<uint32>(hi >> 8) & 0xff,
                       static_cast<uint32>(hi >> 3) & 0x1f,
                       static_cast<uint32>(hi) & 0x3,
                       lo);

                clear_frr (frr);
            }
        }

        // Clear all fault indicators
        write<uint32>(DMAR_FSTS, FSTS_ITE | FSTS_ICE | FSTS_IQE | FSTS_APF | FSTS_AFO | FSTS_PFO);
    }
}

void Dmar::vector (unsigned vector)
{
    unsigned msi = vector - VEC_MSI;

    if (EXPECT_TRUE (msi == 0))
        for (Dmar *dmar = list; dmar; dmar = dmar->next)
            dmar->handle_faults();

    Lapic::eoi();
}

void Rte::set (unsigned b, unsigned d, unsigned f, Ept *e)
{
    Rte *rte = rtp + b;

    if (!rte->present())
        rte->lo = Buddy::ptr_to_phys (new Cte) | RTE_P;

    static_cast<Cte *>(Buddy::phys_to_ptr (rte->addr()))->set (d, f, e);
}

void Cte::set (unsigned d, unsigned f, Ept *e)
{
    Cte *cte = this + d * 8 + f;

    cte->hi = 0;
    cte->lo = Buddy::ptr_to_phys (e) | CTE_P;
}
