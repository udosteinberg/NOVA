/*
 * Extended Page Table (EPT)
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
#include "ept.h"

mword Ept::ord = 8 * sizeof (mword);

Ept *Ept::walk (uint64 gpa, unsigned long l, mword p)
{
    unsigned lev = max;

    for (Ept *e = this;; e = static_cast<Ept *>(Buddy::phys_to_ptr (e->addr()))) {

        e += gpa >> (lev * bpl + PAGE_BITS) & ((1ul << bpl) - 1);

        assert (lev != max || e == this);

        if (lev-- == l)
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

void Ept::update (uint64 gpa, mword o, uint64 hpa, mword a, mword t, bool d)
{
    trace (TRACE_EPT, "EPT:%#010lx GPA:%#010llx O:%#02lx HPA:%#010llx A:%#lx D:%u", Buddy::ptr_to_phys (this), gpa, o, hpa, a, d);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Ept *e = walk (gpa, l, d ? 0 : EPT_R | EPT_W | EPT_X);
    assert (e);

    uint64 v = hpa | t << 3 | a | EPT_I | (l ? EPT_S : 0);

    for (unsigned long i = 1ul << o % bpl; i; i--, e++, v += s) {

        assert (d || !e->present());

        e->set (v);

        trace (TRACE_EPT, "L:%lu EPT:%#010lx HPA:%010lx A:%#05lx S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);
    }
}
