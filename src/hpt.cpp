/*
 * Host Page Table (HPT)
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
#include "hpt.h"

Hpt *Hpt::walk (mword hla, unsigned long l, bool d)
{
    unsigned lev = max;

    for (Hpt *e = this;; e = static_cast<Hpt *>(Buddy::phys_to_ptr (e->addr())) + (hla >> (--lev * bpl + PAGE_BITS) & ((1UL << bpl) - 1))) {

        if (lev == l)
            return e;

        if (!e->present()) {

            if (d)
                return 0;

            e->set (Buddy::ptr_to_phys (new Hpt) | HPT_A | HPT_U | HPT_W | HPT_P);
        }

        else
            assert (!e->super());
    }
}

bool Hpt::update (mword hla, mword o, Paddr hpa, mword a, bool d)
{
    trace (TRACE_PTE, "HPT:%#010lx HLA:%#010lx O:%02lx HPA:%#010lx A:%#lx D:%u", Buddy::ptr_to_phys (this), hla, o, hpa, a, d);

    unsigned long l = o / bpl;
    unsigned long s = 1UL << (l * bpl + PAGE_BITS);

    Hpt *e = walk (hla, l, d);
    assert (e);

    Paddr v = hpa | a | HPT_D | HPT_A | (l ? HPT_S : 0);

    for (unsigned long i = 1UL << o % bpl; i; i--, e++, v += s) {

        assert (d || !e->present());

        e->set (v);

        trace (TRACE_PTE, "L:%lu PTE:%#010lx HPA:%010lx A:%#05lx S:%#lx",
               l,
               Buddy::ptr_to_phys (e),
               e->addr(),
               e->attr(),
               s);
    }

    return l;
}

size_t Hpt::lookup (mword hla, Paddr &phys)
{
    unsigned lev = max;

    for (Hpt *e = this;; e = static_cast<Hpt *>(Buddy::phys_to_ptr (e->addr())) + (hla >> (--lev * bpl + PAGE_BITS) & ((1UL << bpl) - 1))) {

        if (EXPECT_FALSE (!e->present()))
            return 0;

        if (EXPECT_FALSE (lev && !e->super()))
            continue;

        size_t size = 1UL << (lev * bpl + PAGE_BITS);

        phys = e->addr() | (hla & (size - 1));

        return size;
    }
}

bool Hpt::sync_from (Hpt src, mword hla)
{
    Hpt *s = src.walk (hla, max - 1, true), *d = walk (hla, max - 1);

    assert (s);
    assert (d);

    if (d->val == s->val)
        return false;

    d->val = s->val;

    return true;
}

size_t Hpt::sync_master (mword virt)
{
    unsigned lev = max;
    Hpt *pte, *mst;

    for (pte = static_cast<Hpt *>(Buddy::phys_to_ptr (addr())),
         mst = static_cast<Hpt *>(Buddy::phys_to_ptr (reinterpret_cast<mword>(&PDBR)));;
         pte = static_cast<Hpt *>(Buddy::phys_to_ptr (pte->addr())),
         mst = static_cast<Hpt *>(Buddy::phys_to_ptr (mst->addr()))) {

        unsigned shift = --lev * bpl + 12;
        unsigned slot = virt >> shift & ((1UL << bpl) - 1);
        size_t size = 1UL << shift;

        mst += slot;
        pte += slot;

        if (mst->present()) {

            if (slot == (LOCAL_SADDR >> shift & ((1UL << bpl) - 1))) {
                assert (pte->present());
                continue;
            }

            *pte = *mst;
        }

        return size;
    }
}

void Hpt::sync_master_range (mword s_addr, mword e_addr)
{
    while (s_addr < e_addr) {
        size_t size = sync_master (s_addr);
        s_addr = (s_addr & ~(size - 1)) + size;
    }
}

void *Hpt::remap (Paddr phys)
{
    Hpt hpt (current() | HPT_P);

    size_t size = 1UL << (bpl + PAGE_BITS);

    mword offset = phys & (size - 1);

    phys &= ~offset;

    Paddr old;
    if (hpt.lookup (REMAP_SADDR, old)) {
        hpt.update (REMAP_SADDR,        bpl, 0, 0, true);
        hpt.update (REMAP_SADDR + size, bpl, 0, 0, true);
        flush();
    }

    hpt.update (REMAP_SADDR,        bpl, phys,        HPT_W | HPT_P);
    hpt.update (REMAP_SADDR + size, bpl, phys + size, HPT_W | HPT_P);

    return reinterpret_cast<void *>(REMAP_SADDR + offset);
}
