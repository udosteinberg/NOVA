/*
 * Host Page Table (Stage-1)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "extern.hpp"
#include "ptab_hst.hpp"

INIT_PRIORITY (PRIO_PTAB) Ptab_hst Ptab_hst::master { Kmem::sym_to_phys (&PTAB_HPAS) };

bool Ptab_hst::share_from (Ptab_hst src, IAddr v, IAddr o)
{
    bool m; auto i { Tlb::Invalidate::SELF };

    // Sanity check
    assert (v != o);

    // Determine the level at which v and o map to different slots
    auto const l { diverge (v, o) };

    // Walk the src page table to the respective slot at that level (non-allocating)
    auto const s { src.walk (v, l, m = false, i) };
    if (!s) [[unlikely]]
        return false;

    // Walk the dst page table to the respective slot at that level (allocating)
    auto const d { walk (v, l, m = true, i) };
    if (!d) [[unlikely]]
        return false;

    // Atomically read both PTEs
    auto const spte { s->load() };
    auto const dpte { d->load() };

    if (dpte == spte)
        return false;

    d->store (spte);

    return true;
}

void Ptab_hst::share_from_master (IAddr s, IAddr e)
{
    for (; s < e; s += Pte_hst::page_size (diverge (s, MMAP_CPU) * Pte_hst::bpl))
        share_from (master, s, MMAP_CPU);
}

void *Ptab_hst::map (uintptr_t v, OAddr p, Paging::Permissions pm, Memattr ma, unsigned n)
{
    constexpr auto s { Pte_hst::page_size (Pte_hst::bpl) };
    constexpr auto o { Pte_hst::offs_mask (Pte_hst::bpl) };
    bool m { false }; auto i { Tlb::Invalidate::SELF };

    // No PTAB allocation because called from pre-launch code
    auto pte { current().walk (v, 1, m, i) };

    auto const r { v | (p & o) };

    for (p = (p & ~o) | Pte_hst::attr_leaf (1, pm, ma); n--; invalidate (v), pte++, v += s, p += s)
        pte->store (p);

    return reinterpret_cast<void *>(r);
}

void *Ptab_hst::map_tmp (OAddr phys, size_t size, Paging::Permissions pm, Memattr ma, unsigned w)
{
    // Determine remap window boundaries
    uintptr_t v { w ? MMAP_TMP_RW1S : MMAP_TMP_RW0S };
    uintptr_t e { w ? MMAP_TMP_RW1E : MMAP_TMP_RW0E };

    // Round physical address and size to full pages
    auto p { aligned_dn (Pte_hst::page_size (0), phys) };
    auto s { aligned_up (Pte_hst::page_size (0), phys + size) - p };

    // Fail if size is larger than remap window
    if (s > e - v) [[unlikely]]
        return nullptr;

    // Compute return value before v changes
    auto r { reinterpret_cast<void *>(v | (phys & Pte_hst::offs_mask (0))) };

    // Mapping fails if page-table allocation fails
    auto ptab { current() };
    for (unsigned o; s; s -= BITN (o), v += BITN (o), p += BITN (o))
        if (ptab.update (v, p, (o = aligned_order (s, v, p)) - PAGE_BITS, pm, ma) != Status::SUCCESS) [[unlikely]]
            r = nullptr;

    // Invalidate TLB in any case
    invalidate();

    return r;
}
