/*
 * Memory Type Range Registers (MTRR)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "slab.h"
#include "types.h"

class Mtrr
{
    private:
        uint64 const        base;
        uint64 const        mask;
        Mtrr *              next;

        static unsigned     count;
        static unsigned     dtype;
        static Mtrr *       list;
        static Slab_cache   cache;

    public:
        explicit inline Mtrr (uint64, uint64);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        INIT
        static void init();

        INIT
        static unsigned memtype (uint64);
};
