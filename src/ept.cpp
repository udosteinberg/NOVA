/*
 * Extended Page Table (EPT)
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
#include "ept.h"
#include "stdio.h"

Ept *Ept::walk (uint64 gpa, unsigned long l, bool alloc)
{
    unsigned lev = levels;

    for (Ept *e = this;; e = static_cast<Ept *>(Buddy::phys_to_ptr (e->addr()))) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        e += gpa >> shift & ((1ul << bpl) - 1);

        if (lev == l)
            return e;

        if (!e->present()) {

            if (!alloc)
                return 0;

            e->val = Buddy::ptr_to_phys (new Ept) | EPT_R | EPT_W | EPT_X;
        }

        else
            assert (!e->super());
    }
}

void Ept::insert (uint64 gpa, mword o, mword t, mword a, uint64 hpa)
{
    trace (TRACE_EPT, "INS EPT:%#010lx GPA:%#010llx O:%lu HPA:%#010llx A:%#05lx", Buddy::ptr_to_phys (this), gpa, o, hpa, a);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Ept *e = walk (gpa, l, true);
    uint64 v = hpa | t << 3 | a | EPT_I | (l ? EPT_S : EPT_0);

    for (unsigned long i = 1ul << o % bpl; i; i--, e++, v += s) {

        assert (!e->present());

        e->val = v;

        trace (TRACE_EPT, "L:%lu EPT:%#010lx HPA:%010lx A:%#05lx S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);
    }
}

void Ept::remove (uint64 gpa, mword o)
{
    trace (TRACE_EPT, "REM EPT:%#010lx GPA:%#010llx O:%lu", Buddy::ptr_to_phys (this), gpa, o);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Ept *e = walk (gpa, l, false);
    if (!e)
        return;

    for (unsigned long i = 1ul << o % bpl; i; i--, e++) {

        assert (!e->present() || !l || e->super());

        trace (TRACE_EPT, "L:%lu EPT:%#010lx HPA:%010lx A:%#05lx S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);

        e->val = 0;
    }

    flush();
}
