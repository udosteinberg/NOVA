/*
 * I/O Space
 *
 * Copyright (C) 2007-2010, Udo Steinberg <udo@hypervisor.org>
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
                             Ptab::Attribute (Ptab::ATTR_NOEXEC | Ptab::ATTR_WRITABLE),
                             Buddy::ptr_to_phys (bmp));
}

bool Space_io::insert (mword idx)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem()->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_1)) {
        phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_1));
        space_mem()->insert (virt & ~PAGE_MASK, 0,
                             Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                              Ptab::ATTR_WRITABLE),
                             phys);

        phys |= virt & PAGE_MASK;
    }

    // Clear the bit, permitting access
    return Atomic::test_clr_bit<true>(*static_cast<mword *>(Buddy::phys_to_ptr (phys)), idx_to_bit (idx));
}

bool Space_io::remove (mword idx)
{
    Paddr phys;
    if (!space_mem()->lookup (idx_to_virt (idx), phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_1))
        return false;

    // Set the bit, prohibiting access
    return !Atomic::test_set_bit<true>(*static_cast<mword *>(Buddy::phys_to_ptr (phys)), idx_to_bit (idx));
}

bool Space_io::insert (Vma *vma)
{
    if (!vma->insert (&vma_head, &vma_head))
        return false;

    assert (this != &Pd::kern);

    // Whoever owns a VMA struct in the VMA list owns the respective PT slots
    for (unsigned long i = 0; i < (1ul << vma->order); i++)
        insert (vma->base + i);

    return true;
}

void Space_io::page_fault (mword addr, mword error)
{
    // Should never get a write fault during lookup
    assert (!(error & Paging::ERROR_WRITE));

    // First try to sync with PD master page table
    if (Pd::current->Space_mem::sync (addr))
        return;

    // If that didn't work, back the region with a r/o 1-page
    Pd::current->Space_mem::insert (addr & ~PAGE_MASK, 0,
                                    Ptab::ATTR_NOEXEC,
                                    reinterpret_cast<Paddr>(&FRAME_1));
}

void Space_io::insert_root (mword b, unsigned o)
{
    (new Vma (&Pd::kern, b, o))->insert (&vma_head, &vma_head);
}
