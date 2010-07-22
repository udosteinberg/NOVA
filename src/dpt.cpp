/*
 * DMA Page Table (DPT)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "assert.h"
#include "dpt.h"

mword Dpt::ord = ~0UL;

Dpt *Dpt::walk (uint64 dpa, unsigned long l, bool d)
{
    unsigned lev = max;

    for (Dpt *e = this;; e = static_cast<Dpt *>(Buddy::phys_to_ptr (e->addr())), lev--) {

        e += dpa >> (lev * bpl + PAGE_BITS) & ((1ul << bpl) - 1);

        assert (lev != max || e == this);

        if (lev == l)
            return e;

        if (!e->present()) {

            if (d)
                return 0;

            e->set (Buddy::ptr_to_phys (new Dpt) | DPT_R | DPT_W);
        }

        else
            assert (!e->super());
    }
}

void Dpt::update (uint64 dpa, mword o, uint64 hpa, mword a, bool d)
{
    trace (TRACE_DPT, "DPT:%#010lx DPA:%#010llx O:%#02lx HPA:%#010llx A:%#lx D:%u", Buddy::ptr_to_phys (this), dpa, o, hpa, a, d);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Dpt *e = walk (dpa, l, d);
    assert (e);

    uint64 v = hpa | a | (l ? DPT_S : 0);

    for (unsigned long i = 1ul << o % bpl; i; i--, e++, v += s) {

        assert (d || !e->present());

        e->set (v);

        trace (TRACE_DPT, "L:%lu DPT:%#010lx HPA:%010lx A:%#05lx S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);
    }
}
