/*
 * DMA Remapping Unit (DMAR)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "bits.h"
#include "dmar.h"
#include "initprio.h"
#include "lapic.h"
#include "pd.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache  Dmar::cache (sizeof (Dmar), 8);

Dmar *      Dmar::list;
Dmar_ctx *  Dmar::ctx = new Dmar_ctx;
Dmar_irt *  Dmar::irt = new Dmar_irt;
uint32      Dmar::gcmd = GCMD_TE;

Dmar::Dmar (Paddr phys) : reg_base ((hwdev_addr -= PAGE_SIZE) | (phys & PAGE_MASK)), next (0), invq (static_cast<Dmar_qi *>(Buddy::allocator.alloc (ord, Buddy::FILL_0))), invq_idx (0)
{
    Dmar **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;

    Pd::kern.Space_mem::insert (hwdev_addr, 0,
                                Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                                 Ptab::ATTR_GLOBAL      |
                                                 Ptab::ATTR_UNCACHEABLE |
                                                 Ptab::ATTR_WRITABLE),
                                phys & ~PAGE_MASK);

    cap  = read<uint64>(REG_CAP);
    ecap = read<uint64>(REG_ECAP);

    Dpt::ord = min (Dpt::ord, static_cast<unsigned>(bit_scan_reverse (static_cast<mword>(cap >> 34) & 0xf) + 2) * Dpt::bpl - 1);

    write<uint32>(REG_FECTL,  0);
    write<uint32>(REG_FEDATA, VEC_MSI_DMAR);
    write<uint32>(REG_FEADDR, 0xfee00000);

    write<uint64>(REG_RTADDR, Buddy::ptr_to_phys (ctx));
    command (GCMD_SRTP);

    if (ir()) {
        write<uint64>(REG_IRTA, Buddy::ptr_to_phys (irt) | 7);
        command (GCMD_SIRTP);
        gcmd |= GCMD_IRE;
    }

    if (qi()) {
        write<uint64>(REG_IQT, 0);
        write<uint64>(REG_IQA, Buddy::ptr_to_phys (invq));
        command (GCMD_QIE);
        gcmd |= GCMD_QIE;
    }
}

void Dmar::assign (unsigned rid, Pd *p)
{
    assert (p->did && p->dpt);

    mword lev = bit_scan_reverse (read<mword>(REG_CAP) >> 8 & 0x1f);

    Dmar_ctx *r = ctx + (rid >> 8);
    if (!r->present())
        r->set (0, Buddy::ptr_to_phys (new Dmar_ctx) | 1);

    Dmar_ctx *c = static_cast<Dmar_ctx *>(Buddy::phys_to_ptr (r->addr())) + (rid & 0xff);
    if (c->present())
        c->set (0, 0);

    flush_ctx();

    c->set (lev | p->did << 8, Buddy::ptr_to_phys (p->dpt->level (lev + 1)) | 1);
}

void Dmar::fault_handler()
{
    for (uint32 fsts; fsts = read<uint32>(REG_FSTS), fsts & 0xff;) {

        if (fsts & 0x2) {
            uint64 hi, lo;
            for (unsigned frr = fsts >> 8 & 0xff; read (frr, hi, lo), hi & 1ull << 63; frr = (frr + 1) % nfr())
                trace (TRACE_DMAR, "DMAR:%p FRR:%u FR:%#x BDF:%x:%x:%x FI:%#010llx",
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

void Dmar::vector (unsigned vector)
{
    unsigned msi = vector - VEC_MSI;

    if (EXPECT_TRUE (msi == 0))
        for (Dmar *dmar = list; dmar; dmar = dmar->next)
            dmar->fault_handler();

    Lapic::eoi();
}
