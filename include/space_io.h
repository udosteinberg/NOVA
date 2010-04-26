/*
 * I/O Space
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

#pragma once

#include "compiler.h"

class Space_mem;

class Space_io
{
    protected:
        Vma vma_head;

    private:
        ALWAYS_INLINE
        static inline mword idx_to_virt (mword idx)
        {
            return IOBMP_SADDR + (idx / 8 / sizeof (mword)) * sizeof (mword);
        }

        ALWAYS_INLINE
        static inline mword idx_to_bit (mword idx)
        {
            return idx % (8 * sizeof (mword));
        }

        ALWAYS_INLINE
        inline Space_mem *space_mem();

        bool insert (mword);
        bool remove (mword);

    public:
        void * const bmp;

        Space_io (unsigned);

        static bool insert (Vma *);
        static void page_fault (mword, mword);

        INIT
        void insert_root (mword, unsigned);
};
