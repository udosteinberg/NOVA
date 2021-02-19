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

#include "extern.hpp"
#include "pd.hpp"

Space_mem *Space_obj::space_mem()
{
    return static_cast<Pd *>(this);
}

Paddr Space_obj::walk (mword idx)
{
    mword virt = idx_to_virt (idx); uint64 phys; unsigned o;

    if (!space_mem()->lookup (virt, phys, o) || (phys & ~OFFS_MASK) == Kmem::ptr_to_phys (&PAGE_0)) {

        phys = Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0));

        space_mem()->update (virt, phys, 0, Paging::Permissions (Paging::R | Paging::W), Memattr::ram());

        phys |= virt & OFFS_MASK;
    }

    return phys;
}

void Space_obj::update (mword idx, Capability cap)
{
    *static_cast<Capability *>(Kmem::phys_to_ptr (walk (idx))) = cap;
}

size_t Space_obj::lookup (mword idx, Capability &cap)
{
    uint64 phys; unsigned o;
    if (!space_mem()->lookup (idx_to_virt (idx), phys, o) || (phys & ~OFFS_MASK) == Kmem::ptr_to_phys (&PAGE_0))
        return 0;

    cap = *static_cast<Capability *>(Kmem::phys_to_ptr (phys));

    return 1;
}

bool Space_obj::insert_root (Kobject *)
{
    return true;
}

void Space_obj::page_fault (mword addr, mword error)
{
    assert (!(error & BIT (1)));

    if (!Pd::current->Space_mem::loc[Cpu::id].share_from (Pd::current->Space_mem::hpt, addr, MMAP_CPU))
        Pd::current->Space_mem::update (addr, Kmem::ptr_to_phys (&PAGE_0), 0, Paging::R, Memattr::ram());
}
