/*
 * Mapping Database
 *
 * Copyright (C) 2008, Udo Steinberg <udo@hypervisor.org>
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

class Pd;

class Map_node
{
    private:
        static Slab_cache cache;

        Map_node *  prev;
        Map_node *  next;
        unsigned    depth;      // 0 = root

    public:
        // XXX: Should be compressed to save space
        Pd *        pd;         // Which PD does this mapping belong to?
        mword       base;       // at which address does this mapping exist?
        mword       attr;       // attributes
        Spinlock    lock;

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            cache.free (ptr);
        }

        explicit Map_node (Pd *, mword, mword = 0);

        ALWAYS_INLINE
        inline bool root() const
        {
            return !depth;
        }

        Map_node * lookup (Pd *, mword);
        Map_node * remove_child();

        void add_child (Map_node *);
};
