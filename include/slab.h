/*
 * Slab Allocator
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

#include "buddy.h"
#include "initprio.h"

class Slab;

class Slab_cache
{
    private:
        Spinlock    lock;
        Slab *      curr;
        Slab *      head;

        /*
         * Back end allocator
         */
        void grow();

    public:
        unsigned long size; // Size of an element
        unsigned long buff; // Size of an element buffer (includes link field)
        unsigned long elem; // Number of elements

        Slab_cache (unsigned long elem_size, unsigned elem_align);

        /*
         * Front end allocator
         */
        void *alloc();

        /*
         * Front end deallocator
         */
        void free (void *ptr);
};

class Slab
{
    public:
        unsigned long   avail;
        Slab_cache *    cache;
        Slab *          prev;                     // Prev slab in cache
        Slab *          next;                     // Next slab in cache
        char *          head;

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            Buddy::allocator.free (reinterpret_cast<mword>(ptr));
        }

        Slab (Slab_cache *slab_cache);

        ALWAYS_INLINE
        inline bool full() const
        {
            return !avail;
        }

        ALWAYS_INLINE
        inline bool empty() const
        {
            return avail == cache->elem;
        }

        void enqueue();
        void dequeue();

        ALWAYS_INLINE
        inline void *alloc();

        ALWAYS_INLINE
        inline void free (void *ptr);
};
