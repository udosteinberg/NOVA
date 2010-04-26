/*
 * Extended Page Table (EPT)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

unsigned Ept::ord = 8 * sizeof (mword);

Ept *Ept::walk (uint64 gpa, unsigned long l, mword p)
{
    unsigned lev = max;

    for (Ept *e = this;; e = static_cast<Ept *>(Buddy::phys_to_ptr (e->addr()))) {

        e += gpa >> (--lev * bpl + PAGE_BITS) & ((1ul << bpl) - 1);

        if (lev == l)
            return e;

        if (!e->present()) {

            if (!p)
                return 0;

            e->set (Buddy::ptr_to_phys (new Ept) | p);
        }

        else
            assert (!e->super());
    }
}

void Ept::insert (uint64 gpa, mword o, uint64 hpa, mword a, mword t)
{
    trace (TRACE_EPT, "INS EPT:%#010lx GPA:%#010llx O:%lu HPA:%#010llx A:%#05lx", Buddy::ptr_to_phys (this), gpa, o, hpa, a);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Ept *e = walk (gpa, l, EPT_R | EPT_W | EPT_X);
    uint64 v = hpa | t << 3 | a | EPT_I | (l ? EPT_S : 0);

    for (unsigned long i = 1ul << o % bpl; i; i--, e++, v += s) {

        assert (!e->present());

        e->set (v);

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

    Ept *e = walk (gpa, l);
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

        e->set (0);
    }

    flush();
}
