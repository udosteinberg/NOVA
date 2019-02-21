/*
 * Buddy Allocator
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

#include "assert.hpp"
#include "bits.hpp"
#include "buddy.hpp"
#include "initprio.hpp"
#include "lock_guard.hpp"
#include "stdio.hpp"
#include "string.hpp"

extern char _mempool_p, _mempool_l, _mempool_f, _mempool_e;

/*
 * Buddy Allocator
 */
INIT_PRIORITY (PRIO_BUDDY)
Buddy Buddy::allocator (reinterpret_cast<mword>(&_mempool_l),
                        reinterpret_cast<mword>(&_mempool_f),
                        reinterpret_cast<mword>(&_mempool_e) -
                        reinterpret_cast<mword>(&_mempool_l));

Buddy::Buddy (mword virt, mword f_addr, size_t size)
{
    size   -= order * sizeof *head;
    base    = align_dn (virt, 1UL << (PTE_BPL + PAGE_BITS));
    min_idx = page_to_index (virt);
    max_idx = page_to_index (virt + (size / (PAGE_SIZE + sizeof *index)) * PAGE_SIZE);
    head    = reinterpret_cast<Block *>(virt + size);
    index   = reinterpret_cast<Block *>(virt + size) - max_idx;

    for (unsigned i = 0; i < order; i++)
        head[i].next = head[i].prev = head + i;

    for (mword i = f_addr; i < index_to_page (max_idx); i += PAGE_SIZE)
        free (i);
}

/*
 * Allocate physically contiguous memory region.
 * @param ord       Block order (2^ord pages)
 * @param zero      Zero out block content if true
 * @return          Pointer to linear memory region
 */
void *Buddy::alloc (uint16 ord, Fill fill)
{
    Lock_guard <Spinlock> guard (lock);

    for (auto j = ord; j < order; j++) {

        if (head[j].next == head + j)
            continue;

        Block *block = head[j].next;
        block->prev->next = block->next;
        block->next->prev = block->prev;
        block->ord = ord;
        block->tag = Block::Tag::USED;

        while (j-- != ord) {
            Block *buddy = block + (1UL << j);
            buddy->prev = buddy->next = head + j;
            buddy->ord = j;
            buddy->tag = Block::Tag::FREE;
            head[j].next = head[j].prev = buddy;
        }

        mword virt = index_to_page (block_to_index (block));

        // Ensure corresponding physical block is order-aligned
        assert ((virt_to_phys (virt) & ((1UL << (block->ord + PAGE_BITS)) - 1)) == 0);

        if (fill)
            memset (reinterpret_cast<void *>(virt), fill == FILL_0 ? 0 : -1, 1UL << (block->ord + PAGE_BITS));

        return reinterpret_cast<void *>(virt);
    }

    Console::panic ("Out of memory");
}

/*
 * Free physically contiguous memory region.
 * @param virt     Linear block base address
 */
void Buddy::free (mword virt)
{
    auto idx = page_to_index (virt);

    // Ensure virt is within allocator range
    assert (idx >= min_idx && idx < max_idx);

    Block *block = index_to_block (idx);

    // Ensure block is marked as used
    assert (block->tag == Block::Tag::USED);

    // Ensure corresponding physical block is order-aligned
    assert ((virt_to_phys (virt) & ((1UL << (block->ord + PAGE_BITS)) - 1)) == 0);

    Lock_guard <Spinlock> guard (lock);

    uint16 ord;
    for (ord = block->ord; ord < order - 1; ord++) {

        // Compute block index and corresponding buddy index
        auto block_idx = block_to_index (block);
        auto buddy_idx = block_idx ^ (1UL << ord);

        // Buddy outside mempool
        if (buddy_idx < min_idx || buddy_idx >= max_idx)
            break;

        Block *buddy = index_to_block (buddy_idx);

        // Buddy in use or fragmented
        if (buddy->tag == Block::Tag::USED || buddy->ord != ord)
            break;

        // Dequeue buddy from block list
        buddy->prev->next = buddy->next;
        buddy->next->prev = buddy->prev;

        // Merge block with buddy
        if (buddy < block)
            block = buddy;
    }

    block->ord = ord;
    block->tag = Block::Tag::FREE;

    // Enqueue final-size block
    Block *h = head + ord;
    block->prev = h;
    block->next = h->next;
    block->next->prev = h->next = block;
}
