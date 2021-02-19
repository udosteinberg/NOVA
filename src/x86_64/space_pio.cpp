/*
 * Port I/O Space
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

Space_mem *Space_pio::space_mem()
{
    return static_cast<Pd *>(this);
}

Paddr Space_pio::walk (bool host, mword idx)
{
    Paddr &bmp = host ? hbmp : gbmp;

    if (!bmp) {
        bmp = Kmem::ptr_to_phys (Buddy::alloc (1, Buddy::Fill::BITS1));

        if (host)
            space_mem()->update (MMAP_SPC_IOP, bmp, 1, Paging::Permissions (Paging::R | Paging::W), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
    }

    return bmp | (idx_to_virt (idx) & (2 * PAGE_SIZE - 1));
}

void Space_pio::update (bool host, mword idx, mword attr)
{
    mword *m = static_cast<mword *>(Kmem::phys_to_ptr (walk (host, idx)));

    if (attr)
        __atomic_and_fetch (m, ~idx_to_mask (idx), __ATOMIC_SEQ_CST);
    else
        __atomic_or_fetch (m, idx_to_mask (idx), __ATOMIC_SEQ_CST);
}

void Space_pio::page_fault (mword addr, mword error)
{
    assert (!(error & BIT (1)));

    if (!Pd::current->Space_mem::loc[Cpu::id].sync_from (Pd::current->Space_mem::hpt, addr, MMAP_CPU))
        Pd::current->Space_mem::update (addr, Kmem::ptr_to_phys (&PAGE_1), 0, Paging::R, Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}
