/*
 * Protection Domain (PD)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "pd.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), 32);

ALIGNED(32) Pd Pd::kern (USER_ADDR);
ALIGNED(32) Pd Pd::root;

Pd::Pd() : Kobject (Kobject::Type::PD)
{
    trace (TRACE_CREATE, "PD:%p created", static_cast<void *>(this));
}

Pd::Pd (mword size) : Kobject (Kobject::Type::PD)
{
    insert_mem_user (0, LOAD_ADDR);
    insert_mem_user (reinterpret_cast<uint64>(&LOAD_STOP), size - reinterpret_cast<uint64>(&LOAD_STOP));
}

void Pd::update_mem_user (Paddr addr, mword size, bool accessible)
{
    for (unsigned o; size; size -= 1UL << o, addr += 1UL << o)
        kern.Space_mem::update (addr, addr, (o = static_cast<unsigned>(max_order (addr, size))) - PAGE_BITS,
                                Paging::Permissions (accessible ? Paging::R | Paging::W | Paging::X | Paging::U : 0),
                                Memattr::Cacheability::DEV, Memattr::Shareability::NONE);
}

// XXX: Check if src + 1<<ord is not larger than max
void Pd::update_obj_space (Pd *pd, mword src, mword dst, unsigned ord, unsigned pmm)
{
    for (mword ssel = src, dsel = dst; ssel < src + (1UL << ord); ssel++, dsel++) {

        Capability cap = pd->Space_obj::lookup (ssel);

        auto o = cap.obj();
        auto x = cap.prm() & pmm;

        Space_obj::update (dsel, Capability (o, x));
    }
}

// XXX: Check if src + 1<<ord is not larger than max
void Pd::update_mem_space (Pd *pd, mword src, mword dst, unsigned ord, unsigned pmm, Memattr::Cacheability ca, Memattr::Shareability sh, Space::Index si)
{
    unsigned o;

    for (mword ssel = src, dsel = dst; ssel < src + (1UL << ord); ssel += (1UL << o), dsel += (1UL << o)) {

        mword s = ssel << PAGE_BITS;
        mword d = dsel << PAGE_BITS;
        Hpt::OAddr p;
        Memattr::Cacheability src_ca;
        Memattr::Shareability src_sh;

        auto pm = Paging::Permissions (pd->Space_mem::lookup (s, p, o, src_ca, src_sh) & (Paging::U | pmm));

        // If src PD is not kernel, then inherit shareability and memory type
        if (pd != &Pd::kern) {
            sh = src_sh;
            ca = src_ca;
        }

        o = min (o, ord);

        s &= ~Hpt::page_mask (o);
        d &= ~Hpt::page_mask (o);
        p &= ~Hpt::page_mask (o);

        Space_mem::update (d, p, o, pm, ca, sh, si);
    }

    Space_mem::flush (si);
}
