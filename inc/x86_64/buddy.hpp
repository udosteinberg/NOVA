/*
 * Buddy Allocator
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "queue.hpp"
#include "spinlock.hpp"
#include "types.hpp"

class Buddy
{
    private:
        typedef uint8           Order;
        typedef unsigned long   Index;

        // Valid orders range from 0 (PAGE_SIZE) to PTE_BPL (SUPERPAGE_SIZE)
        static constexpr Order orders       { PTE_BPL + 1 };

        class Block : public Queue<Block>::Element
        {
            public:
                enum class Tag : uint8
                {
                    USED,
                    FREE,
                };

                Order   ord { 0 };
                Tag     tag { Tag::USED };
        };

        class Freelist
        {
            private:
                Queue<Block> list[orders];

            public:
                inline void enqueue (Block *b)  { list[b->ord].enqueue_head (b); }
                inline void dequeue (Block *b)  { list[b->ord].dequeue (b); }
                inline auto dequeue (Order o)   { return list[o].dequeue_head(); }
        };

        class Waitlist
        {
            private:
                Queue<Block> list;

            public:
                inline void enqueue (Block *b)  { list.enqueue_head (b); }
                inline auto dequeue()           { return list.dequeue_head(); }
        };

        static inline Spinlock      lock;               // Allocator Spinlock
        static inline Index         min_idx;            // Minimum Block Index
        static inline Index         max_idx;            // Maximum Block Index
        static inline uintptr_t     mem_base;           // Base of Memory Pool
        static inline Block *       blk_base;           // Base of Block Array
        static inline Freelist      freelist;           // Block Freelist

        static Waitlist waitlist    CPULOCAL;           // Block Waitlist (per Core)

        static inline bool is_valid (Index x)           { return x >= min_idx && x < max_idx; }

        static inline auto index_to_block (Index x)     { return blk_base + x; }
        static inline auto block_to_index (Block *x)    { return static_cast<Index>(x - blk_base); }

        static inline auto index_to_page (Index x)      { return mem_base + x * PAGE_SIZE; }
        static inline auto page_to_index (uintptr_t x)  { return static_cast<Index>((x - mem_base) / PAGE_SIZE); }

        NONNULL
        static void coalesce (Block *);

    public:
        enum class Fill
        {
            NONE,
            BITS0,
            BITS1,
        };

        static void init();

        NODISCARD
        static void *alloc (Order, Fill = Fill::NONE);

        NONNULL
        static void free (void *);

        NONNULL
        static void wait (void *);

        static inline void free_wait() { for (Block *b; (b = waitlist.dequeue()); coalesce (b)); }
};
