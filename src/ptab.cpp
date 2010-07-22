/*
 * IA32 Page Table
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
#include "ptab.h"

Paddr Ptab::remap_addr = static_cast<Paddr>(~0ull);

Ptab *Ptab::walk (mword hla, unsigned long l, mword p)
{
    unsigned lev = levels;

    for (Ptab *e = this;; e = static_cast<Ptab *>(Buddy::phys_to_ptr (e->addr()))) {

        e += hla >> (--lev * bpl + PAGE_BITS) & ((1ul << bpl) - 1);

        if (lev == l)
            return e;

        if (!e->present()) {

            if (!p)
                return 0;

            e->set (Buddy::ptr_to_phys (new Ptab) | p);
        }

        else
            assert (!e->super());
    }
}

bool Ptab::update (mword hla, mword o, Paddr hpa, mword a, bool d)
{
    trace (TRACE_PTE, "HPT:%#010lx HLA:%#010lx O:%02lx HPA:%#010lx A:%#lx D:%u", Buddy::ptr_to_phys (this), hla, o, hpa, a, d);

    unsigned long l = o / bpl;
    unsigned long s = 1ul << (l * bpl + PAGE_BITS);

    Ptab *e = walk (hla, l, d ? 0 : ATTR_PTAB);
    assert (e);

    Paddr v = hpa | a | ATTR_DIRTY | ATTR_ACCESSED | (l ? ATTR_SUPERPAGE : 0);

    for (unsigned long i = 1ul << o % bpl; i; i--, e++, v += s) {

        assert (d || !e->present());

        e->set (v);

        trace (TRACE_PTE, "L:%lu PTE:%#010lx HPA:%010lx A:%#05x S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);
    }

    return l;
}

size_t Ptab::lookup (mword virt, Paddr &phys)
{
    unsigned lev = levels;

    for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        pte += virt >> shift & ((1UL << bpl) - 1);

        if (EXPECT_FALSE (!pte->present()))
            return 0;

        if (EXPECT_FALSE (lev && !pte->super()))
            continue;

        size_t size = 1UL << shift;

        phys = pte->addr() | (virt & (size - 1));

        return size;
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
    unsigned const o = 11;
    unsigned offset = static_cast<unsigned>(phys & ((1u << (o + PAGE_BITS)) - 1));

    phys &= ~offset;

    if (phys != remap_addr) {
        update (REMAP_SADDR, o, phys, 0x7, true);
        flush();
        remap_addr = phys;
    }

    return reinterpret_cast<void *>(REMAP_SADDR + offset);
}
