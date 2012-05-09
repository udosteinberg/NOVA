/*
 * I/O Space
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

#include "pd.h"

Space_mem *Space_io::space_mem()
{
    return static_cast<Pd *>(this);
}

Paddr Space_io::walk (mword idx)
{
    if (!bmp)
        space_mem()->insert (SPC_LOCAL_IOP, 1,
                             Hpt::HPT_NX | Hpt::HPT_D | Hpt::HPT_A | Hpt::HPT_W | Hpt::HPT_P,
                             bmp = Buddy::ptr_to_phys (Buddy::allocator.alloc (1, Buddy::FILL_1)));

    return bmp | (idx_to_virt (idx) & (2 * PAGE_SIZE - 1));
}

void Space_io::update (mword idx, mword attr)
{
    mword *m = static_cast<mword *>(Buddy::phys_to_ptr (walk (idx)));

    if (attr)
        Atomic::clr_mask (*m, idx_to_mask (idx));
    else
        Atomic::set_mask (*m, idx_to_mask (idx));
}

void Space_io::update (Mdb *mdb, mword r)
{
    assert (this == mdb->node_pd && this != &Pd::kern);
    Lock_guard <Spinlock> guard (mdb->node_lock);
    for (unsigned long i = 0; i < (1UL << mdb->node_order); i++)
        update (mdb->node_base + i, mdb->node_attr & ~r);
}

void Space_io::page_fault (mword addr, mword error)
{
    assert (!(error & Hpt::ERR_W));

    if (!Pd::current->Space_mem::loc[Cpu::id].sync_from (Pd::current->Space_mem::hpt, addr, CPU_LOCAL))
        Pd::current->Space_mem::replace (addr, reinterpret_cast<Paddr>(&FRAME_1) | Hpt::HPT_NX | Hpt::HPT_A | Hpt::HPT_P);
}
