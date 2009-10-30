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

#pragma once

#include "capability.h"
#include "compiler.h"
#include "types.h"

class Map_node;
class Space_mem;

class Space_obj
{
    private:
        ALWAYS_INLINE
        static inline mword idx_to_virt (unsigned long idx)
        {
            return OBJSP_SADDR + (idx % caps) * sizeof (Capability);
        }

        ALWAYS_INLINE
        static inline Map_node ** shadow (void *ptr)
        {
            return static_cast<Map_node **>(ptr) + PAGE_SIZE / sizeof (Map_node **);
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

        void *metadata (unsigned long);

        bool insert (unsigned long, Capability);
        bool insert (Capability, Map_node *, Map_node *);

        Map_node * lookup_node (mword);

        bool remove (unsigned, Capability);

        static void page_fault (mword, mword);
};
