/*
 * DMA Page Table (DPT)
 *
 * Copyright (C) 2009, Udo Steinberg <udo@hypervisor.org>
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
#include "stdio.h"
#include "x86.h"

Dpt *Dpt::walk (uint64 dpa, unsigned long l, bool alloc)
{
    unsigned lev = levels;

    for (Dpt *e = this;; e = static_cast<Dpt *>(Buddy::phys_to_ptr (e->addr()))) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        e += dpa >> shift & ((1ul << bpl) - 1);

        if (lev == l)
            return e;

        if (!e->present()) {

            if (!alloc)
                return 0;

            e->set (Buddy::ptr_to_phys (new Dpt) | DPT_R | DPT_W);
        }

        else
            assert (!e->super());
    }
}

void Dpt::insert (uint64 dpa, mword o, mword a, uint64 hpa)
{
    trace (TRACE_DPT, "INS DPT:%#010lx DPA:%#010llx O:%lu HPA:%#010llx A:%#05lx", Buddy::ptr_to_phys (this), dpa, o, hpa, a);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Dpt *e = walk (dpa, l, true);
    uint64 v = hpa | a | (l ? DPT_SP : DPT_0);

    for (unsigned long i = 1ul << o % bpl; i; i--, e++, v += s) {

        assert (!e->present());

        e->set (v);

        trace (TRACE_DPT, "L:%lu DPT:%#010lx HPA:%010lx A:%#05lx S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);
    }
}

void Dpt::remove (uint64 dpa, mword o)
{
    trace (TRACE_EPT, "REM DPT:%#010lx DPA:%#010llx O:%lu", Buddy::ptr_to_phys (this), dpa, o);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Dpt *e = walk (dpa, l, false);
    if (!e)
        return;

    for (unsigned long i = 1ul << o % bpl; i; i--, e++) {

        assert (!e->present() || !l || e->super());

        trace (TRACE_DPT, "L:%lu DPT:%#010lx HPA:%010lx A:%#05lx S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);

        e->set (0);
    }

    //flush();
}
