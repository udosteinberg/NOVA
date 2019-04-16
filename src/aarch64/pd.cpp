/*
 * Protection Domain (PD)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "pd_kern.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), Kobject::alignment);

Pd::Pd() : Kobject (Kobject::Type::PD)
{
    trace (TRACE_CREATE, "PD:%p created", static_cast<void *>(this));
}

bool Pd::update_space_obj (Pd *pd, mword src, mword dst, unsigned ord, unsigned pmm)
{
    mword const s_end = src + (1UL << ord), d_end = dst + (1UL << ord);

    if (EXPECT_FALSE (s_end > Space_obj::num || d_end > Space_obj::num))
        return false;

    for (mword s_sel = src, d_sel = dst; s_sel < s_end; s_sel++, d_sel++) {

        Capability cap = pd->Space_obj::lookup (s_sel), old;

        auto o = cap.obj();
        auto x = cap.prm() & pmm;

        // FIXME: Inc refcount for new capability object and dec refcount for old capability object
        Space_obj::update (d_sel, Capability (o, x), old);
    }

    return true;
}

bool Pd::update_space_mem (Pd *pd, mword src, mword dst, unsigned ord, unsigned pmm, Memattr::Cacheability ca, Memattr::Shareability sh, Space::Index si)
{
    mword const s_end = src + (1UL << ord), d_end = dst + (1UL << ord);

    if (EXPECT_FALSE (s_end > Space_mem::num || d_end > Space_mem::num))
        return false;

    unsigned o;

    for (mword s_sel = src, d_sel = dst; s_sel < s_end; s_sel += (1UL << o), d_sel += (1UL << o)) {

        mword s = s_sel << PAGE_BITS;
        mword d = d_sel << PAGE_BITS;
        Hpt::OAddr p;
        Memattr::Cacheability src_ca;
        Memattr::Shareability src_sh;

        auto pm = Paging::Permissions (pd->Space_mem::lookup (s, p, o, src_ca, src_sh) & (Paging::K | Paging::U | pmm));

        // Kernel memory cannot be delegated
        if (pm & Paging::K)
            pm = Paging::Permissions (0);

        // If src PD is not kernel, then inherit shareability and memory type
        if (pd != &Pd_kern::nova()) {
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

    return true;
}
