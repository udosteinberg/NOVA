/*
 * Host Page Table (HPT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "assert.hpp"
#include "atomic.hpp"
#include "bits.hpp"
#include "extern.hpp"
#include "hpt.hpp"

bool Hpt::sync_from (Hpt src, mword v, mword o)
{
    unsigned l = (bit_scan_reverse (v ^ o) - PAGE_BITS) / bpl;

    Hpt *s = static_cast<Hpt *>(src.walk (v, l, false));
    if (!s)
        return false;

    Hpt *d = static_cast<Hpt *>(walk (v, l, true));
    assert (d);

    if (d->val == s->val)
        return false;

    d->val = s->val;

    return true;
}

void Hpt::sync_master_range (mword s, mword e)
{
    for (mword l = (bit_scan_reverse (LINK_ADDR ^ CPU_LOCAL) - PAGE_BITS) / bpl; s < e; s += 1UL << (l * bpl + PAGE_BITS))
        sync_from (Hptp (Kmem::ptr_to_phys (&PTAB_HVAS)), s, CPU_LOCAL);
}

void *Hpt::map (OAddr p, bool w)
{
    Hptp hpt = Hptp::current();

    OAddr s = page_size (bpl);
    OAddr o = page_mask (bpl) & p;

    p &= ~o;

    hpt.update (SPC_LOCAL_REMAP,     p,     bpl, Paging::Permissions ((w ? Paging::W : 0) | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE);
    hpt.update (SPC_LOCAL_REMAP + s, p + s, bpl, Paging::Permissions ((w ? Paging::W : 0) | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE);

    hpt.flush (SPC_LOCAL_REMAP);
    hpt.flush (SPC_LOCAL_REMAP + s);

    return reinterpret_cast<void *>(SPC_LOCAL_REMAP | o);
}
