/*
 * Object Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "pd.hpp"

Space_mem *Space_obj::space_mem()
{
    return static_cast<Pd *>(this);
}

Paddr Space_obj::walk (mword idx)
{
    mword virt = idx_to_virt (idx); Paddr phys; void *ptr;

    if (!space_mem()->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0)) {

        Paddr p = Kmem::ptr_to_phys (ptr = Buddy::allocator.alloc (0, Buddy::FILL_0));

        if ((phys = space_mem()->replace (virt, p | Hpt::HPT_NX | Hpt::HPT_D | Hpt::HPT_A | Hpt::HPT_W | Hpt::HPT_P)) != p)
            Buddy::allocator.free (reinterpret_cast<mword>(ptr));

        phys |= virt & PAGE_MASK;
    }

    return phys;
}

void Space_obj::update (mword idx, Capability cap)
{
    *static_cast<Capability *>(Kmem::phys_to_ptr (walk (idx))) = cap;
}

size_t Space_obj::lookup (mword idx, Capability &cap)
{
    Paddr phys;
    if (!space_mem()->lookup (idx_to_virt (idx), phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0))
        return 0;

    cap = *static_cast<Capability *>(Kmem::phys_to_ptr (phys));

    return 1;
}

void Space_obj::update (Mdb *mdb, mword r)
{
    assert (this == mdb->space && this != &Pd::kern);
    Lock_guard <Spinlock> guard (mdb->node_lock);
    update (mdb->node_base, Capability (reinterpret_cast<Kobject *>(mdb->node_phys), mdb->node_attr & ~r));
}

bool Space_obj::insert_root (Kobject *obj)
{
    if (!obj->space->tree_insert (obj))
        return false;

    if (obj->space != static_cast<Space_obj *>(&Pd::kern))
        static_cast<Space_obj *>(obj->space)->update (obj->node_base, Capability (obj, obj->node_attr));

    return true;
}

void Space_obj::page_fault (mword addr, mword error)
{
    assert (!(error & Hpt::ERR_W));

    if (!Pd::current->Space_mem::loc[Cpu::id].sync_from (Pd::current->Space_mem::hpt, addr, CPU_LOCAL))
        Pd::current->Space_mem::replace (addr, reinterpret_cast<Paddr>(&FRAME_0) | Hpt::HPT_NX | Hpt::HPT_A | Hpt::HPT_P);
}
