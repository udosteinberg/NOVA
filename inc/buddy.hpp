/*
 * Buddy Allocator
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "arch.hpp"
#include "memory.hpp"
#include "spinlock.hpp"
#include "types.hpp"

class Buddy
{
    private:
        typedef uint8 Order;

        // Valid orders range from 0 (page size) to PTE_BPL (superpage size)
        static Order const orders           { PTE_BPL + 1 };

        class Block
        {
            public:
                enum class Tag : uint8
                {
                    USED,
                    FREE,
                };

                Order   ord                 { 0 };
                Tag     tag                 { Tag::USED };
                Block * prev                { nullptr };
                Block * next                { nullptr };
        };

        class Freelist
        {
            private:
                Block * list[orders]        { nullptr };

            public:
                inline auto head (Order o) const { return list[o]; }

                void enqueue (Block *);
                void dequeue (Block *);
        };

        static Spinlock         lock;       // Allocator Spinlock
        static unsigned long    min_idx;    // Minimum Block Index Number
        static unsigned long    max_idx;    // Maximum Block Index Number
        static uintptr_t        base;       // Memory Pool Base (Index 0)
        static uintptr_t        offs;       // Virt/Phys Offset
        static Block *          index;      // Block Index
        static Freelist         flist;      // Block Freelist

        static inline unsigned long block_to_index (Block *x) { return x - index; }
        static inline Block *index_to_block (unsigned long x) { return index + x; }

        static inline unsigned long page_to_index (uintptr_t x) { return (x - base) / PAGE_SIZE; }
        static inline uintptr_t index_to_page (unsigned long x) { return base + x * PAGE_SIZE; }

        static inline Paddr virt_to_phys (uintptr_t virt) { return virt - offs; }
        static inline uintptr_t phys_to_virt (Paddr phys) { return phys + offs; }

    public:
        enum class Fill
        {
            NONE,
            BITS0,
            BITS1,
        };

        static void init (uintptr_t);

        static inline auto phys_to_ptr (Paddr p) { return reinterpret_cast<void *>(phys_to_virt (p)); }
        static inline auto ptr_to_phys (void *p) { return virt_to_phys (reinterpret_cast<uintptr_t>(p)); }

        static inline auto sym_to_virt (void *p) { return reinterpret_cast<uintptr_t>(p) + OFFSET; }
        static inline auto sym_to_phys (void *p) { return virt_to_phys (sym_to_virt (p)); }

        static void *alloc (Order, Fill = Fill::NONE);
        static void free (void *);
};
