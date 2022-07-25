/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "space_dma.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"

template <typename T>
Status Space_mem<T>::delegate (Space_hst const *hst, unsigned long const ssb, unsigned long const dsb, unsigned const ord, unsigned const pmm, Memattr ma)
{
    auto const sse { ssb + BITN (ord) }, dse { dsb + BITN (ord) };

    if (EXPECT_FALSE (sse > hst->selectors() || dse > T::selectors()))
        return Status::BAD_PAR;

    unsigned o;

    auto sts { Status::SUCCESS };

    for (auto src { ssb }, dst { dsb }; src < sse; src += BITN (o), dst += BITN (o)) {

        uintptr_t s { src << PAGE_BITS };
        uintptr_t d { dst << PAGE_BITS };
        Hpt::OAddr p;
        Memattr a;

        auto pm { Paging::Permissions (hst->lookup (s, p, o, a) & (Paging::K | Paging::U | pmm)) };

        // Kernel memory cannot be delegated
        if (pm & Paging::K)
            pm = Paging::NONE;

        // Memory attributes are inherited for virt/virt delegations
        if (hst != &Space_hst::nova)
            ma = a;

        o = min (o, ord);

        s &= ~Hpt::offs_mask (o);
        d &= ~Hpt::offs_mask (o);
        p &= ~Hpt::offs_mask (o);

        if ((sts = static_cast<T *>(this)->update (d, p, o, pm, ma)) != Status::SUCCESS)
            break;
    }

    static_cast<T *>(this)->sync();

    Buddy::free_wait();

    return sts;
}

template class Space_mem<Space_hst>;
template class Space_mem<Space_gst>;
template class Space_mem<Space_dma>;
