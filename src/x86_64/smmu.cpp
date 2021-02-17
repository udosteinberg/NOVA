/*
 * System Memory Management Unit (Intel IOMMU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
#include "pd.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Smmu::cache { sizeof (Smmu), alignof (Smmu) };

Smmu::Smmu (Paddr p) : List (list), phys_base (p), mmio_base (mmap), invq (static_cast<Smmu_qi *>(Buddy::alloc (ord, Buddy::Fill::BITS0)))
{
    mmap += PAGE_SIZE;

    // Reserve MMIO region
    Space_hst::user_access (p, PAGE_SIZE, false);

    Hptp::master_map (mmio_base, phys_base, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::Cacheability::MEM_UC, Memattr::Shareability::NONE);

    cap  = read (Register64::CAP);
    ecap = read (Register64::ECAP);

    // Treat DPT as noncoherent if at least one SMMU requires it
    Dpt::noncoherent |= !has_co();

    ir &= has_ir();

    // DPT maximum leaf page size: 2 + { 1 (1GB), 0 (2MB), -1 (4KB) }
    Dptp::set_leaf_max (2 + bit_scan_reverse (cap >> 34 & BIT_RANGE (1, 0)));

    trace (TRACE_SMMU, "SMMU: %#010lx CAP:%#018llx ECAP:%#018llx C:%u I:%u", phys_base, cap, ecap, has_co(), has_ir());
}

void Smmu::init()
{
    write (Register32::FEADDR, 0xfee00000 | Cpu::apic_id[0] << 12);
    write (Register32::FEDATA, VEC_FLT);
    write (Register32::FECTL,  0);

    if (has_qi()) {
        write (Register64::IQT, 0);
        write (Register64::IQA, Kmem::ptr_to_phys (invq));
        command (Cmd::QIE);
    }

    set_irt (Kmem::ptr_to_phys (irt));
    set_rtp (Kmem::ptr_to_phys (ctx));
}

bool Smmu::configure (Space_dma *dma, uintptr_t dad, bool inv)
{
    auto const bus { static_cast<uint8>(dad >> 8) };
    auto const dfn { static_cast<uint8>(dad) };
    auto const lev { bit_scan_reverse (cap >> 8 & BIT_RANGE (4, 0)) };

    auto const sdid { dma->get_sdid() };
    auto const ptab { dma->get_ptab (lev + 1) };

    if (!ptab)
        return false;

    trace (TRACE_SMMU, "SMMU: %#010lx Device %02x:%02x.%x assigned to DMA:%p", phys_base, bus, dfn >> 3, dfn & BIT_RANGE (2, 0), dma);

    auto const r { ctx + bus };
    if (!r->present())
        r->set (0, Kmem::ptr_to_phys (new Smmu_ctx) | 1);

    auto const c { static_cast<Smmu_ctx *>(Kmem::phys_to_ptr (r->addr())) + dfn };
    if (c->present())
        c->set (0, 0);

    if (inv)
        invalidate_ctx (sdid);

    c->set (lev | sdid << 8, Kmem::ptr_to_phys (ptab) | 1);

    return true;
}

void Smmu::fault_handler()
{
    for (uint32 fsts; fsts = read (Register32::FSTS), fsts & 0xff;) {

        if (fsts & 0x2) {
            uint64 hi, lo;
            for (unsigned frr { fsts >> 8 & 0xff }; read (frr, hi, lo), hi & 1ull << 63; frr = (frr + 1) % nfr())
                trace (TRACE_SMMU, "SMMU: %#010lx FRR:%u FR:%#x BDF:%x:%x:%x FI:%#010llx",
                       phys_base,
                       frr,
                       static_cast<uint32>(hi >> 32) & 0xff,
                       static_cast<uint32>(hi >> 8) & 0xff,
                       static_cast<uint32>(hi >> 3) & 0x1f,
                       static_cast<uint32>(hi) & 0x7,
                       lo);
        }

        write (Register32::FSTS, 0x7d);
    }
}
