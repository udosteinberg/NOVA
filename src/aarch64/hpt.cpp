/*
 * Host Page Table (HPT)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "extern.hpp"
#include "hpt.hpp"

Hptp Hptp::master (Buddy::ptr_to_phys (&PTAB_HVAS));

void Hpt::sync_from_master (mword v, mword)
{
    Hpt *s = Hptp::master.walk (v, lev - 1, false);
    Hpt *d = walk (v, lev - 1, true);

    d->val = s->val;
}

void *Hpt::map (OAddr p, bool w)
{
    Hptp hpt = Hptp::current();

    OAddr s = page_size (bpl);
    OAddr o = page_mask (bpl) & p;

    p &= ~o;

    hpt.update (CPU_LOCAL_TMAP,     p,     bpl, Paging::Permissions ((w ? Paging::W : 0) | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
    hpt.update (CPU_LOCAL_TMAP + s, p + s, bpl, Paging::Permissions ((w ? Paging::W : 0) | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    hpt.flush_cpu();

    return reinterpret_cast<void *>(CPU_LOCAL_TMAP | o);
}
