/*
 * Slab Allocator
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

#include "assert.hpp"
#include "bits.hpp"
#include "buddy.hpp"
#include "lock_guard.hpp"
#include "slab.hpp"

struct Slab_cache::Slab
{
    struct Buffer
    {
        Buffer *        next    { nullptr };    // Intra-Slab Buffer Linkage
    };

    Slab_cache * const  cache;                  // Slab_cache for this Slab
    Slab *              prev    { nullptr };    // Prev Slab in Slab_cache
    Slab *              next    { nullptr };    // Next Slab in Slab_cache
    Buffer *            head    { nullptr };    // Head of Buffer List
    unsigned            acnt    { 0 };          // Available Buffer Count

    inline bool full() const    { return acnt == 0; }
    inline bool empty() const   { return acnt == cache->bps; }

    /*
     * Allocate an element in this slab
     *
     * @return  Pointer to the element
     */
    NODISCARD ALWAYS_INLINE
    inline void *alloc()
    {
        // Unlink buffer
        auto b = head;
        head = head->next;

        // Update available buffer count
        acnt--;

        // The buffer that previously contained a buffer link will now be used as an element
        return static_cast<void *>(b);
    }

    /*
     * Free an element in this slab
     *
     * @param p Pointer to the element
     * @return  true if slab was previously full, false otherwise
     */
    ALWAYS_INLINE
    inline bool free (void *p)
    {
        // The buffer that previously contained an element will now be used as a buffer link
        auto b = static_cast<Buffer *>(p);

        // Relink buffer
        b->next = head;
        head = b;

        // Update available buffer count
        return !acnt++;
    }

    /*
     * Slab Constructor
     *
     * @param c Slab cache to which this slab belongs
     */
    ALWAYS_INLINE
    inline Slab (Slab_cache *c) : cache (c)
    {
        // Free all buffers in the slab
        for (auto i = cache->bps; i; i--)
            free (reinterpret_cast<char *>(this) + PAGE_SIZE - i * cache->bsz);
    }

    NODISCARD
    static inline void *operator new (size_t) noexcept
    {
        return Buddy::alloc (0, Buddy::Fill::BITS0);
    }

    static inline void operator delete (void *ptr)
    {
        if (EXPECT_TRUE (ptr))
            Buddy::free (ptr);
    }
};

/*
 * Slab Cache Constructor
 *
 * @param s Required element size
 * @param a Required element alignment (must be a power of 2)
 *
 * Slab Linkage Example (P:partial precede F:full)
 *
 * nullptr <- P <-> P <-> P <-> P <-> F <-> F -> nullptr
 *            ^                 ^
 *          head              curr
 *
 *  head && !curr => slab cache contains only F-Slabs => no buffer available
 *  head &&  curr => slab cache contains some P-Slabs => buffer in curr available
 * !head && !curr => slab cache contains no slabs => initial state
 * !head &&  curr => illegal
 */
Slab_cache::Slab_cache (size_t s, size_t a) : bsz (static_cast<uint16>(align_up (max (s, sizeof (Slab::Buffer)), max (a, alignof (Slab::Buffer))))),
                                              bps ((PAGE_SIZE - sizeof (Slab)) / bsz) {}

/*
 * Allocate an element in this slab cache
 *
 * @return  Pointer to the element (success) or nullptr (failure)
 */
void *Slab_cache::alloc()
{
    Lock_guard <Spinlock> guard (lock);

    // Cache contains no slabs or only full slabs
    if (EXPECT_FALSE (!curr)) {

        // Allocate a new slab
        auto slab = new Slab (this);

        // Allocation failed
        if (EXPECT_FALSE (!slab))
            return nullptr;

        // Link slab as head and curr (with no predecessor)
        slab->next = head;

        if (head)
            head->prev = slab;

        head = curr = slab;
    }

    // The current slab must be either empty or partial
    assert (!curr->full());

    // If we have a successor slab, it must be full
    assert (!curr->next || curr->next->full());

    // Allocate element in current slab
    auto p = curr->alloc();

    // If the current slab is now full, make its predecessor current
    if (EXPECT_FALSE (curr->full()))
        curr = curr->prev;

    return p;
}

/*
 * Free an element in this slab cache
 *
 * @param p Pointer to the element
 */
void Slab_cache::free (void *p)
{
    Lock_guard <Spinlock> guard (lock);

    // Compute slab for this element
    auto slab = reinterpret_cast<Slab *>(reinterpret_cast<uintptr_t>(p) & ~PAGE_MASK);

    // Ensure we use the correct cache
    assert (slab->cache == this);

    // Free element in slab
    auto was_full = slab->free (p);

    // Slab Transition Full/Partial => Empty
    if (EXPECT_FALSE (slab->empty())) {

        // If the slab was curr, new curr is the slab's predecessor
        if (slab == curr)
            curr = slab->prev;

        // If the slab was head, new head is the slab's successor
        if (slab == head)
            head = slab->next;

        // Unlink slab
        if (slab->prev)
            slab->prev->next = slab->next;
        if (slab->next)
            slab->next->prev = slab->prev;

        // Deallocate slab
        delete slab;

    // Slab Transition Full => Partial
    } else if (EXPECT_FALSE (was_full)) {

        // Slab is now partial and there are full slabs in front it => requeue
        if (slab->prev && slab->prev->full()) {

            // Unlink slab
            slab->prev->next = slab->next;
            if (slab->next)
                slab->next->prev = slab->prev;

            if (curr) {         // Link as successor of curr
                slab->prev = curr;
                slab->next = curr->next;
                curr->next = curr->next->prev = slab;
            } else {            // Link as head
                slab->prev = nullptr;
                slab->next = head;
                head = head->prev = slab;
            }
        }

        curr = slab;
    }
}
