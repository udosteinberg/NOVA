/*
 * Host Page Table (HPT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
    unsigned l = (bit_scan_reverse (v ^ o) - PAGE_BITS) / Hpt::bpl;

    auto d = walk (v, l, true);
    if (!d)
        return false;

    auto s = src.walk (v, l, false);
    if (!s)
        return false;

    auto dpte = static_cast<Hpt>(*d);
    auto spte = static_cast<Hpt>(*s);

    if (dpte == spte)
        return false;

    *d = spte;

    return true;
}

void *Hptp::map (OAddr p, bool w)
{
    auto c = current();
    auto s = Hpt::page_size (Hpt::bpl);
    auto o = Hpt::offs_mask (Hpt::bpl) & p;

    p &= ~o;

    c.update (MMAP_CPU_TMAP,     p,     Hpt::bpl, Paging::Permissions ((w ? Paging::W : 0) | Paging::R), Memattr::ram());
    c.update (MMAP_CPU_TMAP + s, p + s, Hpt::bpl, Paging::Permissions ((w ? Paging::W : 0) | Paging::R), Memattr::ram());

    invalidate (MMAP_CPU_TMAP);
    invalidate (MMAP_CPU_TMAP + s);

    return reinterpret_cast<void *>(MMAP_CPU_TMAP | o);
}
