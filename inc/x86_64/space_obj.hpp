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

#pragma once

#include "capability.hpp"
#include "space.hpp"

class Space_mem;

class Space_obj : public Space
{
    private:
        ALWAYS_INLINE
        static inline mword idx_to_virt (unsigned long idx)
        {
            return SPC_LOCAL_OBJ + (idx % caps) * sizeof (Capability);
        }

        ALWAYS_INLINE
        inline Space_mem *space_mem();

        void update (mword, Capability);

    public:
        static unsigned const caps = (END_SPACE_LIM - SPC_LOCAL_OBJ) / sizeof (Capability);

        ALWAYS_INLINE
        static inline Capability lookup (unsigned long idx)
        {
            return *reinterpret_cast<Capability *>(idx_to_virt (idx));
        }

        size_t lookup (mword, Capability &);

        Paddr walk (mword = 0);

        static void page_fault (mword, mword);

        static bool insert_root (Kobject *);
};
