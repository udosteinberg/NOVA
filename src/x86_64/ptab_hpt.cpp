/*
 * Host Page Table (HPT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
#include "ptab_hpt.hpp"

INIT_PRIORITY (PRIO_PTAB) Hptp Hptp::master { Kmem::ptr_to_phys (&PTAB_HVAS) };

bool Hptp::share_from (Hptp src, IAddr v, IAddr o)
{
    // Sanity check
    assert (v != o);

    // Determine the level at which v and o map to different slots
    auto const l { diverge (v, o) };

    // Walk the src page table to the respective slot at that level (non-allocating)
    auto const s { src.walk (v, l, false) };
    if (!s) [[unlikely]]
        return false;

    // Walk the dst page table to the respective slot at that level (allocating)
    auto const d { walk (v, l, true) };
    if (!d) [[unlikely]]
        return false;

    // Atomically read both PTEs
    auto const spte { static_cast<Hpt>(*s) };
    auto const dpte { static_cast<Hpt>(*d) };

    if (dpte == spte)
        return false;

    *d = spte;

    return true;
}

void Hptp::share_from_master (IAddr s, IAddr e)
{
    for (; s < e; s += Hpt::page_size (diverge (s, MMAP_CPU) * Hpt::bpl))
        share_from (master, s, MMAP_CPU);
}

void *Hptp::map (uintptr_t v, OAddr p, Paging::Permissions pm, Memattr ma, unsigned n)
{
    constexpr auto s { Hpt::page_size (Hpt::bpl) };
    constexpr auto o { Hpt::offs_mask (Hpt::bpl) };

    // No PTAB allocation because called from pre-launch code
    auto pte { current().walk (v, 1, false) };

    auto const r { v | (p & o) };

    for (p = (p & ~o) | Hpt::page_attr (1, pm, ma); n--; invalidate (v), pte++, v += s, p += s)
        *pte = p;

    return reinterpret_cast<void *>(r);
}

void *Hptp::map_tmp (OAddr phys, size_t size, Paging::Permissions pm, Memattr ma, unsigned w)
{
    // Determine remap window boundaries
    uintptr_t v { w ? MMAP_TMP_RW1S : MMAP_TMP_RW0S };
    uintptr_t e { w ? MMAP_TMP_RW1E : MMAP_TMP_RW0E };

    // Round physical address and size to full pages
    auto p { aligned_dn (Hpt::page_size (0), phys) };
    auto s { aligned_up (Hpt::page_size (0), phys + size) - p };

    // Fail if size is larger than remap window
    if (s > e - v) [[unlikely]]
        return nullptr;

    // Compute return value before v changes
    auto r { reinterpret_cast<void *>(v | (phys & Hpt::offs_mask (0))) };

    // Mapping fails if page-table allocation fails
    auto ptab { current() };
    for (unsigned o; s; s -= BITN (o), v += BITN (o), p += BITN (o))
        if (ptab.update (v, p, (o = aligned_order (s, v, p)) - PAGE_BITS, pm, ma) != Status::SUCCESS) [[unlikely]]
            r = nullptr;

    // Invalidate TLB in any case
    invalidate();

    return r;
}
