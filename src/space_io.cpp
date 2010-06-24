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

Space_io::Space_io (unsigned flags) : bmp (flags & 0x1 ? Buddy::allocator.alloc (1, Buddy::FILL_1) : 0)
{
    if (bmp)
        space_mem()->insert (IOBMP_SADDR, 1,
                             Ptab::Attribute (Ptab::ATTR_NOEXEC | Ptab::ATTR_WRITABLE | Ptab::ATTR_PRESENT),
                             Buddy::ptr_to_phys (bmp));
}

void Space_io::update (mword idx, mword attr)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem()->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_1)) {
        phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_1));
        space_mem()->insert (virt & ~PAGE_MASK, 0,
                             Ptab::Attribute (Ptab::ATTR_NOEXEC | Ptab::ATTR_WRITABLE | Ptab::ATTR_PRESENT),
                             phys);

        phys |= virt & PAGE_MASK;
    }

    if (attr)
        Atomic::clr_mask<true>(*static_cast<mword *>(Buddy::phys_to_ptr (phys)), idx_to_mask (idx));
    else
        Atomic::set_mask<true>(*static_cast<mword *>(Buddy::phys_to_ptr (phys)), idx_to_mask (idx));
}

void Space_io::update (Mdb *mdb, bool, mword rem)
{
    assert (this == mdb->node_pd && this != &Pd::kern);
    Lock_guard <Spinlock> guard (mdb->node_lock);
    for (unsigned long i = 0; i < (1UL << mdb->node_order); i++)
        update (mdb->node_base + i, mdb->node_attr & ~rem);
}

void Space_io::insert_root (mword b, unsigned o)
{
    insert_node (new Mdb (&Pd::kern, b, o, 7));
}

void Space_io::page_fault (mword addr, mword error)
{
    // Should never get a write fault during lookup
    assert (!(error & Paging::ERROR_WRITE));

    // First try to sync with PD master page table
    if (Pd::current->Space_mem::sync_mst (addr))
        return;

    // If that didn't work, back the region with a r/o 1-page
    Pd::current->Space_mem::insert (addr & ~PAGE_MASK, 0,
                                    Ptab::Attribute (Ptab::ATTR_NOEXEC | Ptab::ATTR_PRESENT),
                                    reinterpret_cast<Paddr>(&FRAME_1));
}
