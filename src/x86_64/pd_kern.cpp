/*
 * Kernel Protection Domain (PD)
 *
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

#include "extern.hpp"
#include "mtrr.hpp"
#include "pd_kern.hpp"

ALIGNED(Kobject::alignment) Pd_kern Pd_kern::singleton;

Pd_kern::Pd_kern()
{
    hpt = Hptp::master;

    Mtrr::init();

    auto s = Kmem::sym_to_phys (&NOVA_HPAS);
    auto e = Kmem::sym_to_phys (&NOVA_HPAE);

    insert_user_mem (0, s);
    insert_user_mem (e, (Space_mem::num << PAGE_BITS) - e);
    insert_user_pio (0, BIT (16));
}

void Pd_kern::update_user_mem (Paddr addr, mword size, bool accessible)
{
    for (unsigned o; size; size -= 1UL << o, addr += 1UL << o)
        Space_mem::update (addr, addr, (o = static_cast<unsigned>(max_order (addr, size))) - PAGE_BITS,
                           accessible ? Paging::Permissions (Paging::U | Paging::API) : Paging::NONE,
                           Memattr::Cacheability::MEM_UC, Memattr::Shareability::NONE);
}

void Pd_kern::update_user_pio (mword port, mword size, bool accessible)
{
    // FIXME: Optimize
    for (mword i = 0; i < size; i++)
        Space_pio::update (port + i, accessible ? Paging::R : Paging::NONE, Space_pio::bmp_hst());
}
