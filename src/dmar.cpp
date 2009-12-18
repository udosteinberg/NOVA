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

Slab_cache      Dmar::cache (sizeof (Dmar), 8);
Dmar *          Dmar::list;
Dmar_context *  Dmar::root = new Dmar_context;

Dmar::Dmar (Paddr phys) : reg_base ((hwdev_addr -= PAGE_SIZE) | (phys & PAGE_MASK)), next (0)
{
    Dmar **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;

    Pd::kern.Space_mem::insert (hwdev_addr, 0,
                                Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                                 Ptab::ATTR_GLOBAL      |
                                                 Ptab::ATTR_UNCACHEABLE |
                                                 Ptab::ATTR_WRITABLE),
                                phys & ~PAGE_MASK);

    uint64 cap  = read<uint64>(REG_CAP);
    uint64 ecap = read<uint64>(REG_ECAP);

    frr_count = static_cast<unsigned>(cap >> 40 & 0xff) + 1;
    frr_base  = static_cast<mword>(cap >> 20 & 0x3ff0) + reg_base;
    tlb_base  = static_cast<mword>(ecap >> 4 & 0x3ff0) + reg_base;
}

void Dmar::enable()
{
    write<uint32>(REG_FECTL,  0);
    write<uint32>(REG_FEDATA, VEC_MSI_DMAR);
    write<uint32>(REG_FEADDR, 0xfee00000);
    write<uint64>(REG_RTADDR, Buddy::ptr_to_phys (root));

    write<uint32>(REG_GCMD, 1ul << 30);
    while (!(read<uint32>(REG_GSTS) & (1ul << 30)))
        Cpu::pause();

    write<uint64>(REG_CCMD, 1ull << 63 | 1ull << 61);
    while (read<uint64>(REG_CCMD) & (1ull << 63))
        Cpu::pause();

    write<uint64>(REG_IOTLB, 1ull << 63 | 1ull << 60);
    while (read<uint64>(REG_IOTLB) & (1ull << 63))
        Cpu::pause();

    write<uint32>(REG_GCMD, 1ul << 31);
}

void Dmar::assign (unsigned b, unsigned d, unsigned f, Pd *p)
{
    assert (p->dpt());

    mword lev = bit_scan_reverse (read<mword>(REG_CAP) >> 8 & 0x1f);
    mword did = 1;

    Dmar_context *r = root + b;
    if (!r->present())
        r->set (0, Buddy::ptr_to_phys (new Dmar_context) | 1);

    Dmar_context *c = static_cast<Dmar_context *>(Buddy::phys_to_ptr (r->addr())) + d * 8 + f;
    if (!c->present())
        c->set (lev | did << 8, Buddy::ptr_to_phys (p->dpt()->level (lev + 1)) | 1);
}

void Dmar::fault_handler()
{
    for (uint32 fsts; fsts = read<uint32>(REG_FSTS), fsts & 0xff;) {

        if (fsts & 0x2) {
            uint64 hi, lo;
            for (unsigned frr = fsts >> 8 & 0xff; read (frr, hi, lo), hi & 1ull << 63; frr = (frr + 1) % frr_count)
                trace (TRACE_DMAR, "DMAR:%p FRR:%u FR:%#x BDF:%x:%x:%x FI:%#010llx",
                       this,
                       frr,
                       static_cast<uint32>(hi >> 32) & 0xff,
                       static_cast<uint32>(hi >> 8) & 0xff,
                       static_cast<uint32>(hi >> 3) & 0x1f,
                       static_cast<uint32>(hi) & 0x3,
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
