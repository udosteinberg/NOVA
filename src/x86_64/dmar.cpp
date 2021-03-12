/*
 * DMA Remapping Unit (DMAR)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
#include "dmar.hpp"
#include "lapic.hpp"
#include "pd.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache  Dmar::cache (sizeof (Dmar), 8);

Dmar *      Dmar::list;
Dmar_ctx *  Dmar::ctx = new Dmar_ctx;
Dmar_irt *  Dmar::irt = new Dmar_irt;
uint32      Dmar::gcmd = GCMD_TE;

Dmar::Dmar (Paddr p) : List<Dmar> (list), reg_base ((hwdev_addr -= PAGE_SIZE) | (p & OFFS_MASK)), invq (static_cast<Dmar_qi *>(Buddy::alloc (ord, Buddy::Fill::BITS0))), invq_idx (0)
{
#if 0   // FIXME
    Pd::kern.Space_mem::delreg (p & ~OFFS_MASK);
#endif

    Hptp::master_map (reg_base, p & ~OFFS_MASK, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    cap  = read<uint64>(REG_CAP);
    ecap = read<uint64>(REG_ECAP);

    Dptp::set_mll (static_cast<unsigned>(bit_scan_reverse (cap >> 34 & 0xf) + 1));

    write<uint32>(REG_FEADDR, 0xfee00000 | Lapic::id[0] << 12);
    write<uint32>(REG_FEDATA, VEC_FLT);
    write<uint32>(REG_FECTL,  0);

    write<uint64>(REG_RTADDR, Kmem::ptr_to_phys (ctx));
    command (GCMD_SRTP);

    if (ir()) {
        write<uint64>(REG_IRTA, Kmem::ptr_to_phys (irt) | 7);
        command (GCMD_SIRTP);
        gcmd |= GCMD_IRE;
    }

    if (qi()) {
        write<uint64>(REG_IQT, 0);
        write<uint64>(REG_IQA, Kmem::ptr_to_phys (invq));
        command (GCMD_QIE);
        gcmd |= GCMD_QIE;
    }
}

void Dmar::assign (unsigned long rid, Pd *p)
{
    auto lev = static_cast<unsigned>(bit_scan_reverse (read<mword>(REG_CAP) >> 8 & 0x1f));

    auto ptab = p->Space_dma::get_ptab (lev + 1);
    if (!ptab)
        return;

    Dmar_ctx *r = ctx + (rid >> 8);
    if (!r->present())
        r->set (0, Kmem::ptr_to_phys (new Dmar_ctx) | 1);

    Dmar_ctx *c = static_cast<Dmar_ctx *>(Kmem::phys_to_ptr (r->addr())) + (rid & 0xff);
    if (c->present())
        c->set (0, 0);

    flush_ctx();

    c->set (lev | p->get_sdid() << 8, Kmem::ptr_to_phys (ptab) | 1);
}

void Dmar::fault_handler()
{
    for (uint32 fsts; fsts = read<uint32>(REG_FSTS), fsts & 0xff;) {

        if (fsts & 0x2) {
            uint64 hi, lo;
            for (unsigned frr = fsts >> 8 & 0xff; read (frr, hi, lo), hi & 1ull << 63; frr = (frr + 1) % nfr())
                trace (TRACE_SMMU, "DMAR:%p FRR:%u FR:%#x BDF:%x:%x:%x FI:%#010llx",
                       this,
                       frr,
                       static_cast<uint32>(hi >> 32) & 0xff,
                       static_cast<uint32>(hi >> 8) & 0xff,
                       static_cast<uint32>(hi >> 3) & 0x1f,
                       static_cast<uint32>(hi) & 0x7,
                       lo);
        }

        write<uint32>(REG_FSTS, 0x7d);
    }
}
