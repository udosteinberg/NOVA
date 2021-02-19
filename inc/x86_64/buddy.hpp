/*
 * Buddy Allocator
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
#include "bits.hpp"
#include "memory.hpp"
#include "queue.hpp"
#include "spinlock.hpp"

class Buddy final
{
    public:
        using order_t = uint8_t;

        // Maximum page allocation order depends on ALIGNMENT_NOVA
        static constexpr order_t max_order { ALIGNMENT_NOVA - PAGE_BITS };

    private:
        using index_t = unsigned long;

        class Block final : public Queue<Block>::Element
        {
            public:
                enum class Tag : uint8_t
                {
                    USED,
                    FREE,
                };

                order_t ord { 0 };
                Tag     tag { Tag::USED };
        };

        class Freelist final
        {
            private:
                Queue<Block> list[max_order + 1];       // 0 ... max_order

            public:
                void enqueue (Block *b) { list[b->ord].enqueue_head (b); }
                void dequeue (Block *b) { list[b->ord].dequeue (b); }
                auto dequeue (order_t o) { return list[o].dequeue_head(); }
        };

        class Waitlist final
        {
            private:
                Queue<Block> list;

            public:
                void enqueue (Block *b) { list.enqueue_head (b); }
                auto dequeue()          { return list.dequeue_head(); }
        };

        static inline constinit Spinlock    lock;       // Allocator Spinlock
        static inline constinit index_t     min_idx;    // Minimum Block Index
        static inline constinit index_t     max_idx;    // Maximum Block Index
        static inline constinit Block *     blk_base;   // Base of Block Array (index min_idx)
        static inline constinit Freelist    freelist;   // Block Freelist

        static Waitlist waitlist CPULOCAL;              // Block Waitlist (per Core)

        static bool valid (index_t x) { return x >= min_idx && x < max_idx; }

        static auto index_to_block (index_t x)  { return blk_base + (x - min_idx); }
        static auto block_to_index (Block *x)   { return static_cast<index_t>(x - blk_base) + min_idx; }

        static auto index_to_page (index_t x)   { return static_cast<uintptr_t>(LINK_ADDR + x * PAGE_SIZE (0)); }
        static auto page_to_index (uintptr_t x) { return static_cast<index_t>((x - LINK_ADDR) / PAGE_SIZE (0)); }

        NONNULL static void coalesce (Block *);

    public:
        enum class Fill
        {
            NONE,
            BITS0,
            BITS1,
        };

        static void init();

        /*
         * Compute the allocation order needed to cover a power-of-2 byte size
         */
        ALWAYS_INLINE static constexpr auto size_to_ord (size_t s)
        {
            assert (is_power_of_2 (s));

            return static_cast<order_t>(max (PAGE_BITS, bit_scan_msb (s)) - PAGE_BITS);
        }

        [[nodiscard]] static void *alloc (order_t, Fill = Fill::NONE);

        static void free (void *);
        static void wait (void *);

        static void free_wait() { for (Block *b; (b = waitlist.dequeue()); coalesce (b)); }
};
