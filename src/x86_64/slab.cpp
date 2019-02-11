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

#include "assert.hpp"
#include "bits.hpp"
#include "lock_guard.hpp"
#include "slab.hpp"
#include "stdio.hpp"

Slab::Slab (Slab_cache *slab_cache)
    : avail (slab_cache->elem),
      cache (slab_cache),
      prev  (nullptr),
      next  (nullptr),
      head  (nullptr)
{
    char *link = reinterpret_cast<char *>(this) + PAGE_SIZE - cache->buff + cache->size;

    for (unsigned long i = avail; i; i--, link -= cache->buff) {
        *reinterpret_cast<char **>(link) = head;
        head = link;
    }
}

void *Slab::alloc()
{
    avail--;

    void *link = reinterpret_cast<void *>(head - cache->size);
    head = *reinterpret_cast<char **>(head);
    return link;
}

void Slab::free (void *ptr)
{
    avail++;

    char *link = reinterpret_cast<char *>(ptr) + cache->size;
    *reinterpret_cast<char **>(link) = head;
    head = link;
}

Slab_cache::Slab_cache (unsigned long elem_size, unsigned elem_align)
          : curr (nullptr),
            head (nullptr),
            size (align_up (elem_size, sizeof (mword))),
            buff (align_up (size + sizeof (mword), elem_align)),
            elem ((PAGE_SIZE - sizeof (Slab)) / buff)
{
    trace (TRACE_MEMORY, "Slab Cache:%p (S:%lu A:%u)",
           this,
           elem_size,
           elem_align);
}

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
