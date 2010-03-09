/*
 * Object Space
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

#pragma once

#include "capability.h"
#include "compiler.h"
#include "vma.h"

class Space_mem;

class Space_obj
{
    protected:
        Vma vma_head;

    private:
        ALWAYS_INLINE
        static inline mword idx_to_virt (unsigned long idx)
        {
            return OBJSP_SADDR + (idx % caps) * sizeof (Capability);
        }

        ALWAYS_INLINE
        inline Space_mem *space_mem();

    public:
        static unsigned const caps = (OBJSP_EADDR - OBJSP_SADDR) / sizeof (Capability);

        ALWAYS_INLINE
        static inline Capability lookup (unsigned long idx)
        {
            return *reinterpret_cast<Capability *>(idx_to_virt (idx));
        }

        bool    insert (mword, Capability);
        bool    remove (mword, Capability);
        size_t  lookup (mword, Capability &);

        bool insert (Vma *, Capability);

        static void page_fault (mword, mword);

        INIT
        void insert_root (mword);
};
