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

#include "assert.hpp"
#include "bits.hpp"
#include "lock_guard.hpp"
#include "slab.hpp"

Slab::Slab (Slab_cache *c) : cache (c)
{
    for (unsigned long i = cache->elem; i; i--)
        free (reinterpret_cast<char *>(this) + PAGE_SIZE - i * cache->buff);
}

void *Slab::alloc()
{
    avail--;

    char *ptr = reinterpret_cast<char *>(head) - cache->size;
    head = head->link;
    return ptr;
}

void Slab::free (void *ptr)
{
    avail++;

    Slab_elem *e = reinterpret_cast<Slab_elem *>(reinterpret_cast<mword>(ptr) + cache->size);
    e->link = head;
    head = e;
}

Slab_cache::Slab_cache (size_t s, size_t a) : size (align_up (s, alignof (Slab_elem))),
                                              buff (align_up (size + sizeof (Slab_elem), a)),
                                              elem ((PAGE_SIZE - sizeof (Slab)) / buff) {}

void Slab_cache::grow()
{
    Slab *slab = new Slab (this);

    if (head)
        head->prev = slab;

    slab->next = head;
    head = curr = slab;
}

void *Slab_cache::alloc()
{
    Lock_guard <Spinlock> guard (lock);

    if (EXPECT_FALSE (!curr))
        grow();

    assert (!curr->full());
    assert (!curr->next || curr->next->full());

    // Allocate from slab
    void *ret = curr->alloc();

    if (EXPECT_FALSE (curr->full()))
        curr = curr->prev;

    return ret;
}

void Slab_cache::free (void *ptr)
{
    Lock_guard <Spinlock> guard (lock);

    Slab *slab = reinterpret_cast<Slab *>(reinterpret_cast<mword>(ptr) & ~PAGE_MASK);

    bool was_full = slab->full();

    slab->free (ptr);       // Deallocate from slab

    if (EXPECT_FALSE (was_full)) {

        // There are full slabs in front of us and we're partial; requeue
        if (slab->prev && slab->prev->full()) {

            // Dequeue
            slab->prev->next = slab->next;
            if (slab->next)
                slab->next->prev = slab->prev;

            // Enqueue after curr
            if (curr) {
                slab->prev = curr;
                slab->next = curr->next;
                curr->next = curr->next->prev = slab;
            }

            // Enqueue as head
            else {
                slab->prev = nullptr;
                slab->next = head;
                head = head->prev = slab;
            }
        }

        curr = slab;

    } else if (EXPECT_FALSE (slab->empty())) {

        // There are partial slabs in front of us and we're empty; requeue
        if (slab->prev && !slab->prev->empty()) {

            // Make partial slab in front of us current if we were current
            if (slab == curr)
                curr = slab->prev;

            // Dequeue
            slab->prev->next = slab->next;
            if (slab->next)
                slab->next->prev = slab->prev;

            // Enqueue as head
            slab->prev = nullptr;
            slab->next = head;
            head = head->prev = slab;
        }
    }
}
