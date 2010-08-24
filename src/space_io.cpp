/*
 * I/O Space
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

#include "pd.h"

Space_mem *Space_io::space_mem()
{
    return static_cast<Pd *>(this);
}

Paddr Space_io::walk (mword idx)
{
    if (!bmp)
        space_mem()->insert (IOBMP_SADDR, 1,
                             Hpt::HPT_NX | Hpt::HPT_W | Hpt::HPT_P,
                             bmp = Buddy::ptr_to_phys (Buddy::allocator.alloc (1, Buddy::FILL_1)));

    return bmp | (idx_to_virt (idx) & (2 * PAGE_SIZE - 1));
}

void Space_io::update (mword idx, mword attr)
{
    mword *m = static_cast<mword *>(Buddy::phys_to_ptr (walk (idx)));

    if (attr)
        Atomic::clr_mask<true>(*m, idx_to_mask (idx));
    else
        Atomic::set_mask<true>(*m, idx_to_mask (idx));
}

void Space_io::update (Mdb *mdb, bool, mword r, mword)
{
    assert (this == mdb->node_pd && this != &Pd::kern);
    Lock_guard <Spinlock> guard (mdb->node_lock);
    for (unsigned long i = 0; i < (1UL << mdb->node_order); i++)
        update (mdb->node_base + i, mdb->node_attr & ~r);
}

void Space_io::insert_root (mword b, unsigned o)
{
    insert_node (new Mdb (&Pd::kern, b, o, 7));
}

void Space_io::page_fault (mword addr, mword error)
{
    // Should never get a write fault during lookup
    assert (!(error & 2));

    // First try to sync with PD master page table
    if (Pd::current->Space_mem::sync_mst (addr))
        return;

    // If that didn't work, back the region with a r/o 1-page
    Pd::current->Space_mem::insert (addr & ~PAGE_MASK, 0,
                                    Hpt::HPT_NX | Hpt::HPT_P,
                                    reinterpret_cast<Paddr>(&FRAME_1));
}
