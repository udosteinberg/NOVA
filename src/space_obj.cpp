/*
 * Object Space
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "atomic.h"
#include "extern.h"
#include "mdb.h"
#include "pd.h"
#include "regs.h"
#include "space_obj.h"

Space_mem *Space_obj::space_mem()
{
    return static_cast<Pd *>(this);
}

Map_node *Space_obj::lookup_node (mword idx)
{
    mword virt = idx_to_virt (idx);

    size_t size; Paddr phys;
    if (!space_mem()->lookup (virt, size, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0))
        return 0;

    Map_node *node = *shadow (Buddy::phys_to_ptr (phys));

    // This slot is used, but not mappable
    if (node == reinterpret_cast<Map_node *>(~0ul))
        return 0;

    return node;
}

void *Space_obj::metadata (unsigned long idx)
{
    mword virt = idx_to_virt (idx);

    size_t size; Paddr phys;
    if (!space_mem()->lookup (virt, size, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0)) {
        phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (1, Buddy::FILL_0));
        space_mem()->insert (virt & ~PAGE_MASK, 0,
                             Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                              Ptab::ATTR_WRITABLE),
                             phys);

        phys |= virt & PAGE_MASK;
    }

    return Buddy::phys_to_ptr (phys);
}

/*
 * Insert root capability
 */
bool Space_obj::insert (unsigned long idx, Capability cap)
{
    void *ptr = metadata (idx);

    // Store capability only if shadow slot is empty
    if (!Atomic::cmp_swap<true>(shadow (ptr), static_cast<Map_node *>(0), reinterpret_cast<Map_node *>(~0ul)))
        return false;

    *static_cast<Capability *>(ptr) = cap;

    return true;
}

/*
 * Insert capability
 */
bool Space_obj::insert (Capability cap, Map_node *parent, Map_node *child)
{
    void *ptr = metadata (child->base);

    // Store capability only if shadow slot is empty
    if (!Atomic::cmp_swap<true>(shadow (ptr), static_cast<Map_node *>(0), child))
        return false;

    // Link into mapping tree; parent is 0 if root capability
    if (parent)
        parent->add_child (child);

    // Child became visible when we linked it with the parent. Therefore we
    // should hold the child lock while populating cap tables.
    *static_cast<Capability *>(ptr) = cap;

    return true;
}

bool Space_obj::remove (unsigned idx, Capability cap)
{
    mword virt = idx_to_virt (idx);

    size_t size; Paddr phys;
    if (!space_mem()->lookup (virt, size, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0))
        return false;

    // cmpxchg entry from cap slot (see if expected old entry was still there)
    // if we succeeded, just zero out shadow slot

    // Remove capability only if still present
    return Atomic::cmp_swap<true>(static_cast<Capability *>(Buddy::phys_to_ptr (phys)), cap, Capability());
}

void Space_obj::page_fault (mword addr, mword error)
{
    // Should never get a write fault during lookup
    assert (!(error & Paging::ERROR_WRITE));

    // First try to sync with PD master page table
    if (Pd::current->Space_mem::sync (addr))
        return;

    // If that didn't work, back the region with a r/o 0-page
    Pd::current->Space_mem::insert (addr & ~PAGE_MASK, 0,
                                    Ptab::ATTR_NOEXEC,
                                    reinterpret_cast<Paddr>(&FRAME_0));
}
