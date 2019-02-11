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

#pragma once

#include "space.hpp"

class Space_mem;

class Space_pio : public Space
{
    private:
        Paddr hbmp, gbmp;

        ALWAYS_INLINE
        static inline mword idx_to_virt (mword idx)
        {
            return SPC_LOCAL_IOP + (idx / 8 / sizeof (mword)) * sizeof (mword);
        }

        ALWAYS_INLINE
        static inline mword idx_to_mask (mword idx)
        {
            return 1UL << (idx % (8 * sizeof (mword)));
        }

        ALWAYS_INLINE
        inline Space_mem *space_mem();

        void update (bool, mword, mword);

    public:
        Paddr walk (bool = false, mword = 0);

        void update (Mdb *, mword = 0);

        static void page_fault (mword, mword);
};
