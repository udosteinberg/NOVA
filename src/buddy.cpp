/*
 * Buddy Allocator
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "assert.hpp"
#include "bits.hpp"
#include "buddy.hpp"
#include "extern.hpp"
#include "kmem.hpp"
#include "lock_guard.hpp"
#include "multiboot.hpp"
#include "string.hpp"

Buddy::Waitlist Buddy::waitlist;

/*
 * Initialize the buddy allocator
 */
void Buddy::init()
{
    auto const virt { reinterpret_cast<uintptr_t>(&KMEM_HVAS) };
    auto const size { reinterpret_cast<uintptr_t>(Kmem::phys_to_ptr (Multiboot::ea)) - virt };

    mem_base = align_dn (virt, PAGE_SIZE (1));
    min_idx  = page_to_index (virt);
    max_idx  = page_to_index (virt + (size / (PAGE_SIZE (0) + sizeof (Block))) * PAGE_SIZE (0));
    blk_base = reinterpret_cast<Block *>(virt + size) - max_idx;

    // Free all pages in the pool
    for (auto i { reinterpret_cast<uintptr_t>(&KMEM_HVAF) }; i < index_to_page (max_idx); i += PAGE_SIZE (0))
        free (reinterpret_cast<void *>(i));
}

/*
 * Allocate physically and virtually contiguous memory region
 *
 * @param ord       Block order (2^ord pages)
 * @param fill      Fill pattern for the block
 * @return          Pointer to virtual memory region or nullptr if unsuccessful
 */
void *Buddy::alloc (order_t ord, Fill fill)
{
    Lock_guard <Spinlock> guard { lock };

    // Iterate over all freelists, starting with the requested order
    for (auto o { ord }; o < orders; o++) {

        // Get the first block from the order(o) freelist
        auto const block { freelist.dequeue (o) };

        // If that freelist was empty, try higher orders
        if (!block)
            continue;

        // Split higher-order blocks and put the upper half back into the freelist
        while (o-- != ord) {
            auto const buddy { block + BIT (o) };
            assert (buddy->ord == o);
            freelist.enqueue (buddy);
        }

        // Set final block size and mark block as used
        block->ord = ord;
        block->tag = Block::Tag::USED;

        auto const ptr { reinterpret_cast<void *>(index_to_page (block_to_index (block))) };

        // Fill the block if requested
        if (fill != Fill::NONE)
            memset (ptr, fill == Fill::BITS0 ? 0 : ~0U, BIT (block->ord + PAGE_BITS));

        return ptr;
    }

    // Out of memory
    return nullptr;
}

/*
 * Coalesce to-be-freed block
 *
 * @param block     Pointer to the block
 */
void Buddy::coalesce (Block *block)
{
    Lock_guard <Spinlock> guard { lock };

    // Ensure block was used
    assert (block->tag == Block::Tag::USED);

    // Mark block as free
    block->tag = Block::Tag::FREE;

    // Coalesce adjacent order(o) blocks into an order(o+1) block
    for (auto o { block->ord }; o < orders - 1; block->ord = ++o) {

        // Compute buddy index
        auto const buddy_idx { block_to_index (block) ^ BIT (o) };

        // Stop if buddy is outside mempool
        if (!valid (buddy_idx))
            break;

        auto const buddy { index_to_block (buddy_idx) };

        // Stop if buddy is not free or fragmented
        if (buddy->tag != Block::Tag::FREE || buddy->ord != o)
            break;

        // Dequeue buddy from the freelist
        freelist.dequeue (buddy);

        // Merge block with buddy
        if (block > buddy)
            block = buddy;
    }

    // Put final-size block into the freelist
    freelist.enqueue (block);
}

/*
 * Free physically and virtually contiguous memory region immediately
 *
 * @param ptr       Pointer to virtual memory region (or nullptr)
 */
void Buddy::free (void *ptr)
{
    if (EXPECT_FALSE (!ptr))
        return;

    auto const idx { page_to_index (reinterpret_cast<uintptr_t>(ptr)) };

    // Ensure memory is within allocator range
    assert (valid (idx));

    // Coalesce to-be-freed block
    coalesce (index_to_block (idx));
}

/*
 * Free physically and virtually contiguous memory region deferred
 *
 * @param ptr       Pointer to virtual memory region (or nullptr)
 */
void Buddy::wait (void *ptr)
{
    if (EXPECT_FALSE (!ptr))
        return;

    auto const idx { page_to_index (reinterpret_cast<uintptr_t>(ptr)) };

    // Ensure memory is within allocator range
    assert (valid (idx));

    // Waitlist to-be-freed block
    waitlist.enqueue (index_to_block (idx));
}
