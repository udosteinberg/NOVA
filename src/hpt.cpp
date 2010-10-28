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

bool Hpt::sync_from (Hpt src, mword hla)
{
    Hpt *s = static_cast<Hpt *>(src.walk (hla, max() - 1, true)), *d = static_cast<Hpt *>(walk (hla, max() - 1));

    assert (s);
    assert (d);

    if (d->val == s->val)
        return false;

    d->val = s->val;

    return true;
}

size_t Hpt::sync_master (mword virt)
{
    unsigned lev = max();
    Hpt *pte, *mst;

    for (pte = static_cast<Hpt *>(Buddy::phys_to_ptr (addr())),
         mst = static_cast<Hpt *>(Buddy::phys_to_ptr (reinterpret_cast<mword>(&PDBR)));;
         pte = static_cast<Hpt *>(Buddy::phys_to_ptr (pte->addr())),
         mst = static_cast<Hpt *>(Buddy::phys_to_ptr (mst->addr()))) {

        unsigned shift = --lev * bpl() + 12;
        unsigned slot = virt >> shift & ((1UL << bpl()) - 1);
        size_t size = 1UL << shift;

        mst += slot;
        pte += slot;

        if (mst->present()) {

            if (slot == (LOCAL_SADDR >> shift & ((1UL << bpl()) - 1))) {
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
    Hptp hpt (current() | HPT_P);

    size_t size = 1UL << (bpl() + PAGE_BITS);

    mword offset = phys & (size - 1);

    phys &= ~offset;

    Paddr old;
    if (hpt.lookup (REMAP_SADDR, old)) {
        hpt.update (REMAP_SADDR,        bpl(), 0, 0, true);
        hpt.update (REMAP_SADDR + size, bpl(), 0, 0, true);
        flush();
    }

    hpt.update (REMAP_SADDR,        bpl(), phys,        HPT_W | HPT_P);
    hpt.update (REMAP_SADDR + size, bpl(), phys + size, HPT_W | HPT_P);

    return reinterpret_cast<void *>(REMAP_SADDR + offset);
}
