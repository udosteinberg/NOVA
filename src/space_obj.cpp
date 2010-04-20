/*
 * Object Space
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

#include "pd.h"

Space_mem *Space_obj::space_mem()
{
    return static_cast<Pd *>(this);
}

bool Space_obj::insert (mword idx, Capability cap)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem()->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0)) {
        phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0));
        space_mem()->insert (virt & ~PAGE_MASK, 0,
                             Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                              Ptab::ATTR_WRITABLE),
                             phys);

        phys |= virt & PAGE_MASK;
    }

    // Insert capability only if slot empty
    return Atomic::cmp_swap<true>(static_cast<Capability *>(Buddy::phys_to_ptr (phys)), Capability(), cap);
}

bool Space_obj::remove (mword idx, Capability cap)
{
    Paddr phys;
    if (!space_mem()->lookup (idx_to_virt (idx), phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0))
        return false;

    // Remove capability only if still present
    return Atomic::cmp_swap<true>(static_cast<Capability *>(Buddy::phys_to_ptr (phys)), cap, Capability());
}

size_t Space_obj::lookup (mword idx, Capability &cap)
{
    Paddr phys;
    if (!space_mem()->lookup (idx_to_virt (idx), phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0))
        return 0;

    cap = *static_cast<Capability *>(Buddy::phys_to_ptr (phys));

    return 1;
}

void Space_obj::insert (Mdb *mdb, Capability cap)
{
    mdb->node_pd->Space_obj::insert (mdb->node_base, cap);
}

bool Space_obj::insert_root (Kobject *obj)
{
    if (!obj->node_pd->Space_obj::insert_node (obj))
        return false;

    return obj->node_pd->Space_obj::insert (obj->node_base, Capability (obj));
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
