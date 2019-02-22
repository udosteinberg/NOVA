/*
 * Slab Allocator
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "assert.hpp"
#include "buddy.hpp"
#include "initprio.hpp"

class Slab;

class Slab_cache
{
    private:
        Spinlock    lock;
        Slab *      curr    { nullptr };
        Slab *      head    { nullptr };

        /*
         * Back end allocator
         */
        void grow();

    public:
        size_t const        size;   // Size of an element
        size_t const        buff;   // Size of an element buffer (includes Slab_elem)
        unsigned long const elem;   // Number of elements per slab

        Slab_cache (size_t, size_t);

        void *alloc();

        void free (void *);
};

class Slab_elem
{
    public:
        Slab_elem *         link;
};

class Slab
{
    private:
        Slab_cache * const  cache;
        unsigned            avail   { 0 };
        Slab_elem *         head    { nullptr };

    public:
        Slab *              prev    { nullptr };
        Slab *              next    { nullptr };

        static inline void *operator new (size_t) noexcept
        {
            auto ptr = Buddy::alloc (0, Buddy::Fill::BITS0);
            assert (ptr);
            return ptr;
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                Buddy::free (ptr);
        }

        Slab (Slab_cache *);

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

        ALWAYS_INLINE
        inline void *alloc();

        ALWAYS_INLINE
        inline void free (void *ptr);
};
