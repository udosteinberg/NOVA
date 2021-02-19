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

#include "assert.hpp"
#include "bits.hpp"
#include "buddy.hpp"
#include "lock_guard.hpp"
#include "macros.hpp"
#include "string.hpp"

/*
 * Initialize the buddy allocator
 */
void Buddy::init()
{
    extern uintptr_t KMEM_HVAS, KMEM_HVAF, KMEM_HVAE;

    auto const virt = reinterpret_cast<uintptr_t>(&KMEM_HVAS);
    auto const size = reinterpret_cast<uintptr_t>(&KMEM_HVAE) - virt;

    mem_base = align_dn (virt, BIT (PTE_BPL + PAGE_BITS));
    min_idx  = page_to_index (virt);
    max_idx  = page_to_index (virt + (size / (PAGE_SIZE + sizeof (Block))) * PAGE_SIZE);
    blk_base = reinterpret_cast<Block *>(virt + size) - max_idx;

    // Free all pages in the pool
    for (auto i = reinterpret_cast<uintptr_t>(&KMEM_HVAF); i < index_to_page (max_idx); i += PAGE_SIZE)
        free (reinterpret_cast<void *>(i));
}

/*
 * Allocate physically and virtually contiguous memory region
 *
 * @param ord       Block order (2^ord pages)
 * @param fill      Fill pattern for the block
 * @return          Pointer to virtual memory region or nullptr if unsuccessful
 */
void *Buddy::alloc (Order ord, Fill fill)
{
    Lock_guard <Spinlock> guard (lock);

    // Iterate over all freelists, starting with the requested order
    for (auto o = ord; o < orders; o++) {

        // Get the first block from the order(o) freelist
        auto block = freelist.dequeue (o);

        // If that freelist was empty, try higher orders
        if (!block)
            continue;

        // Split higher-order blocks and put the upper half back into the freelist
        while (o-- != ord) {
            auto buddy = block + BIT (o);
            assert (buddy->ord == o);
            freelist.enqueue (buddy);
        }

        // Set final block size and mark block as used
        block->ord = ord;
        block->tag = Block::Tag::USED;

        auto ptr = reinterpret_cast<void *>(index_to_page (block_to_index (block)));

        // Fill the block if requested
        if (fill != Fill::NONE)
            memset (ptr, fill == Fill::BITS0 ? 0 : ~0U, BIT (block->ord + PAGE_BITS));

        return ptr;
    }

    return nullptr;
}

/*
 * Free physically and virtually contiguous memory region
 *
 * @param ptr       Pointer to virtual memory region
 */
void Buddy::free (void *ptr)
{
    auto idx = page_to_index (reinterpret_cast<uintptr_t>(ptr));

    // Ensure idx is within allocator range
    assert (idx >= min_idx && idx < max_idx);

    auto block = index_to_block (idx);

    // Ensure block is marked as used
    assert (block->tag == Block::Tag::USED);

    Lock_guard <Spinlock> guard (lock);

    // Mark block as free
    block->tag = Block::Tag::FREE;

    // Coalesce adjacent order(o) blocks into an order(o+1) block
    for (auto o = block->ord; o < orders - 1; block->ord = ++o) {

        // Compute buddy index
        auto buddy_idx = block_to_index (block) ^ BIT (o);

        // Stop if buddy is outside mempool
        if (buddy_idx < min_idx || buddy_idx >= max_idx)
            break;

        auto buddy = index_to_block (buddy_idx);

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
