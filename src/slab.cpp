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
        Buffer * const next;                        // Intra-Slab Buffer Linkage

        ALWAYS_INLINE
        inline Buffer (Buffer *n) : next (n) {}

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t, void *p) noexcept { return p; }
    };

    struct Metadata
    {
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
            head = new (p) Buffer (head);

            // Update available buffer count
            return !acnt++;
        }

        /*
         * Slab Metadata Constructor
         *
         * @param c Slab cache to which this slab belongs
         * @param s Pointer to end of data storage for this slab
         */
        ALWAYS_INLINE
        inline Metadata (Slab_cache *c, uint8 *s) : cache (c)
        {
            // Free all buffers in the slab
            for (auto i = cache->bps; i; i--)
                free (s - i * cache->bsz);
        }
    };

    Metadata    meta;
    uint8       data[PAGE_SIZE - sizeof (meta)];

    /*
     * Slab Constructor
     *
     * @param c Slab cache to which this slab belongs
     */
    ALWAYS_INLINE
    inline Slab (Slab_cache *c) : meta (c, data + sizeof (data)) {}

    /*
     * Convert buffer pointer to slab pointer
     *
     * @param p Pointer to the buffer
     * @return  Slab to which the specified buffer belongs
     */
    ALWAYS_INLINE
    static inline auto from_buffer (void *p)
    {
        return reinterpret_cast<Slab *>(reinterpret_cast<uintptr_t>(p) & ~PAGE_MASK);
    }

    NODISCARD ALWAYS_INLINE
    static inline void *operator new (size_t) noexcept
    {
        return Buddy::alloc (0, Buddy::Fill::BITS0);
    }

    ALWAYS_INLINE
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
                                              bps ((PAGE_SIZE - sizeof (Slab::Metadata)) / bsz) {}

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
        slab->meta.next = head;

        if (head)
            head->meta.prev = slab;

        head = curr = slab;
    }

    // The current slab must be either empty or partial
    assert (!curr->meta.full());

    // If we have a successor slab, it must be full
    assert (!curr->meta.next || curr->meta.next->meta.full());

    // Allocate element in current slab
    auto p = curr->meta.alloc();

    // If the current slab is now full, make its predecessor current
    if (EXPECT_FALSE (curr->meta.full()))
        curr = curr->meta.prev;

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
    auto slab = Slab::from_buffer (p);

    // Ensure we use the correct cache
    assert (slab->meta.cache == this);

    // Free element in slab
    auto was_full = slab->meta.free (p);

    // Slab Transition Full/Partial => Empty
    if (EXPECT_FALSE (slab->meta.empty())) {

        // If the slab was curr, new curr is the slab's predecessor
        if (slab == curr)
            curr = slab->meta.prev;

        // If the slab was head, new head is the slab's successor
        if (slab == head)
            head = slab->meta.next;

        // Unlink slab
        if (slab->meta.prev)
            slab->meta.prev->meta.next = slab->meta.next;
        if (slab->meta.next)
            slab->meta.next->meta.prev = slab->meta.prev;

        // Deallocate slab
        delete slab;

    // Slab Transition Full => Partial
    } else if (EXPECT_FALSE (was_full)) {

        // Slab is now partial and there are full slabs in front it => requeue
        if (slab->meta.prev && slab->meta.prev->meta.full()) {

            // Unlink slab
            slab->meta.prev->meta.next = slab->meta.next;
            if (slab->meta.next)
                slab->meta.next->meta.prev = slab->meta.prev;

            if (curr) {         // Link as successor of curr
                slab->meta.prev = curr;
                slab->meta.next = curr->meta.next;
                curr->meta.next = curr->meta.next->meta.prev = slab;
            } else {            // Link as head
                slab->meta.prev = nullptr;
                slab->meta.next = head;
                head = head->meta.prev = slab;
            }
        }

        curr = slab;
    }
}
