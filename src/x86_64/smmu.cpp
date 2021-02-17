/*
 * System Memory Management Unit (Intel IOMMU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
#include "lapic.hpp"
#include "pd.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache  Smmu::cache (sizeof (Smmu), 8);

Smmu::Smmu (Paddr p) : List (list), phys_base (p), reg_base (mmap | (p & OFFS_MASK)), invq (static_cast<Smmu_qi *>(Buddy::alloc (ord, Buddy::Fill::BITS0))), invq_idx (0)
{
#if 0   // FIXME
    Pd::kern.Space_mem::delreg (p & ~OFFS_MASK);
#endif

    Hptp::master.update (reg_base, p & ~OFFS_MASK, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::Cacheability::MEM_UC, Memattr::Shareability::NONE);

    cap  = read<uint64>(REG_CAP);
    ecap = read<uint64>(REG_ECAP);

    Dptp::set_leaf_max (static_cast<unsigned>(bit_scan_reverse (cap >> 34 & 0xf) + 2));

    write<uint32>(REG_FEADDR, 0xfee00000 | Cpu::apic_id[0] << 12);
    write<uint32>(REG_FEDATA, VEC_MSI_SMMU);
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

    mmap += PAGE_SIZE;
}

bool Smmu::configure (Pd *pd, Space::Index, mword dev)
{
    auto bus = static_cast<uint8> (dev >> 8);
    auto dfn = static_cast<uint8> (dev);
    auto lev = static_cast<unsigned>(bit_scan_reverse (read<mword>(REG_CAP) >> 8 & 0x1f));

    auto ptab = pd->dpt.root_init (true, lev + 1);
    if (!ptab)
        return false;

    Smmu_ctx *r = ctx + bus;
    if (!r->present())
        r->set (0, Kmem::ptr_to_phys (new Smmu_ctx) | 1);

    Smmu_ctx *c = static_cast<Smmu_ctx *>(Kmem::phys_to_ptr (r->addr())) + dfn;
    if (c->present())
        c->set (0, 0);

    invalidate_ctx();

    c->set (lev | pd->vpid() << 8, Kmem::ptr_to_phys (ptab) | 1);

    return true;
}

void Smmu::fault_handler()
{
    for (uint32 fsts; fsts = read<uint32>(REG_FSTS), fsts & 0xff;) {

        if (fsts & 0x2) {
            uint64 hi, lo;
            for (unsigned frr = fsts >> 8 & 0xff; read (frr, hi, lo), hi & 1ull << 63; frr = (frr + 1) % nfr())
                trace (TRACE_SMMU, "SMMU:%p FRR:%u FR:%#x BDF:%x:%x:%x FI:%#010llx",
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

void Smmu::vector (unsigned vector)
{
    unsigned msi = vector - VEC_MSI;

    if (EXPECT_TRUE (msi == 0))
        for (Smmu *smmu = list; smmu; smmu = smmu->next)
            smmu->fault_handler();

    Lapic::eoi();
}
