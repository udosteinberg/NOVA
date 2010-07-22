/*
 * Object Space
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

Space_mem *Space_obj::space_mem()
{
    return static_cast<Pd *>(this);
}

void Space_obj::update (mword idx, Capability cap)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem()->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0)) {
        phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0));
        space_mem()->insert (virt & ~PAGE_MASK, 0,
                             Ptab::Attribute (Ptab::ATTR_NOEXEC | Ptab::ATTR_WRITABLE | Ptab::ATTR_PRESENT),
                             phys);

        phys |= virt & PAGE_MASK;
    }

    *static_cast<Capability *>(Buddy::phys_to_ptr (phys)) = cap;
}

size_t Space_obj::lookup (mword idx, Capability &cap)
{
    Paddr phys;
    if (!space_mem()->lookup (idx_to_virt (idx), phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0))
        return 0;

    cap = *static_cast<Capability *>(Buddy::phys_to_ptr (phys));

    return 1;
}

void Space_obj::update (Mdb *mdb, Kobject *obj, mword r, mword)
{
    assert (this == mdb->node_pd && this != &Pd::kern);
    Lock_guard <Spinlock> guard (mdb->node_lock);
    update (mdb->node_base, Capability (obj, mdb->node_attr & ~r));
}

bool Space_obj::insert_root (Kobject *obj)
{
    if (!obj->node_pd->Space_obj::insert_node (obj))
        return false;

    obj->node_pd->Space_obj::update (obj->node_base, Capability (obj, obj->node_attr));

    return true;
}

void Space_obj::page_fault (mword addr, mword error)
{
    // Should never get a write fault during lookup
    assert (!(error & Paging::ERROR_WRITE));

    // First try to sync with PD master page table
    if (Pd::current->Space_mem::sync_mst (addr))
        return;

    // If that didn't work, back the region with a r/o 0-page
    Pd::current->Space_mem::insert (addr & ~PAGE_MASK, 0,
                                    Ptab::Attribute (Ptab::ATTR_NOEXEC | Ptab::ATTR_PRESENT),
                                    reinterpret_cast<Paddr>(&FRAME_0));
}
