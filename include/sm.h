/*
 * Semaphore
 *
 * Copyright (C) 2009, Udo Steinberg <udo@hypervisor.org>
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
#include "kobject.h"
#include "mdb.h"
#include "queue.h"
#include "slab.h"
#include "types.h"

class Pd;

class Sm : public Kobject, public Queue
{
    private:
        mword counter;

        static Slab_cache cache;

    public:
        Map_node node;

        Sm (Pd *, mword, mword);

        ALWAYS_INLINE
        inline void dn()
        {
            // Whole function must be atomic on SMP
            if (counter)
                counter--;
            else
                block();
        }

        ALWAYS_INLINE
        inline void up()
        {
            // Whole function must be atomic on SMP
            if (!release())
                counter++;
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
