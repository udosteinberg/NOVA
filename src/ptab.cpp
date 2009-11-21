/*
 * IA32 Page Table
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "buddy.h"
#include "memory.h"
#include "pd.h"
#include "ptab.h"
#include "stdio.h"

Paddr Ptab::remap_addr = static_cast<Paddr>(~0ull);

Ptab *Ptab::walk (mword hla, unsigned long l, bool alloc)
{
    unsigned lev = levels;

    for (Ptab *p = this;; p = static_cast<Ptab *>(Buddy::phys_to_ptr (p->addr()))) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        p += hla >> shift & ((1ul << bpl) - 1);

        if (lev == l)
            return p;

        if (!p->present()) {

            if (!alloc)
                return 0;

            p->val = Buddy::ptr_to_phys (new Ptab) | ATTR_PTAB;
        }

        else
            assert (!p->super());
    }
}

void Ptab::insert (mword hla, mword o, mword a, Paddr hpa)
{
    trace (TRACE_PTE, "INS PTE:%#010lx HLA:%#010lx O:%lu HPA:%#010lx A:%#05lx", Buddy::ptr_to_phys (this), hla, o, hpa, a);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Ptab *p = walk (hla, l, true);
    Paddr v = hpa | a | ATTR_LEAF | (l ? ATTR_SUPERPAGE : 0);

    for (unsigned long i = 1ul << o % bpl; i; i--, p++, v += s) {

        assert (!p->present());

        p->val = v;

        trace (TRACE_PTE, "L:%lu PTE:%#010lx HPA:%010lx A:%#05x S:%#lx",
               l,
               Buddy::ptr_to_phys (p),
               p->addr(),
               p->attr(),
               s);
    }
}

void Ptab::remove (mword hla, mword o)
{
    trace (TRACE_PTE, "REM PTE:%#010lx HLA:%#010lx O:%lu", Buddy::ptr_to_phys (this), hla, o);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Ptab *p = walk (hla, l, false);
    if (!p)
        return;

    for (unsigned long i = 1ul << o % bpl; i; i--, p++) {

        assert (!p->present() || !l || p->super());

        trace (TRACE_PTE, "L:%lu PTE:%#010lx HPA:%010lx A:%#05x S:%#lx",
               l,
               Buddy::ptr_to_phys (p),
               p->addr(),
               p->attr(),
               s);

        p->val = 0;
    }

    flush();
}

bool Ptab::lookup (mword virt, size_t &size, Paddr &phys)
{
    unsigned lev = levels;

    for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

        unsigned shift = --lev * bpl + 12;
        pte += virt >> shift & ((1ul << bpl) - 1);
        size = 1ul << shift;

        if (EXPECT_FALSE (!pte->present()))
            return false;

        if (EXPECT_FALSE (lev && !pte->super()))
            continue;

        phys = pte->addr() | (virt & (size - 1));

        return true;
    }
}

bool Ptab::sync_from (Ptab *src, mword hla)
{
    Ptab *dst = this;

    assert (src);

    unsigned slot = hla >> ((levels - 1) * bpl + PAGE_BITS) & ((1ul << bpl) - 1);

    src += slot;
    dst += slot;

    // Both mappings are identical (or not present)
    if (src->val == dst->val)
        return false;

    *dst = *src;

    return true;
}

size_t Ptab::sync_master (mword virt)
{
    unsigned lev = levels;
    Ptab *pte, *mst;

    for (pte = this, mst = Ptab::master();;
         pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr())),
         mst = static_cast<Ptab *>(Buddy::phys_to_ptr (mst->addr()))) {

        unsigned shift = --lev * bpl + 12;
        unsigned slot = virt >> shift & ((1ul << bpl) - 1);
        size_t size = 1ul << shift;

        mst += slot;
        pte += slot;

        if (mst->present()) {

            if (slot == (LOCAL_SADDR >> shift & ((1ul << bpl) - 1))) {
                assert (pte->present());
                continue;
            }

            *pte = *mst;
        }

        return size;
    }
}

void Ptab::sync_master_range (mword s_addr, mword e_addr)
{
    while (s_addr < e_addr) {
        size_t size = sync_master (s_addr);
        s_addr = (s_addr & ~(size - 1)) + size;
    }
}

void *Ptab::remap (Paddr phys)
{
    unsigned offset = static_cast<unsigned>(phys & ((1u << 22) - 1));

    phys &= ~offset;

    if (phys != remap_addr) {
        remove (REMAP_SADDR, 11);
        insert (REMAP_SADDR, 11, ATTR_WRITABLE, phys);
        remap_addr = phys;
    }

    return reinterpret_cast<void *>(REMAP_SADDR + offset);
}
