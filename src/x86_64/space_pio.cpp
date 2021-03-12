/*
 * PIO Space
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

#include "buddy.hpp"
#include "pd.hpp"

struct Space_pio::Bitmap
{
    static constexpr auto sels = BITN (16);
    static constexpr auto bits = 8 * sizeof (uintptr_t);

    uintptr_t port[sels / bits] { ~0UL };   // I/O port bitmap

    static auto sel_to_mask (unsigned long s) { return BITN (s % bits); }
    static auto sel_to_word (unsigned long s) { return s % sels / bits; }

    /*
     * Allocate PIO bitmap
     *
     * @return      Pointer to the bitmap (allocation success) or nullptr (allocation failure)
     */
    NODISCARD
    static inline void *operator new (size_t) noexcept
    {
        static_assert (sizeof (Bitmap) == PAGE_SIZE * 2);
        return Buddy::alloc (1);
    }

    /*
     * Deallocate PIO bitmap
     *
     * @param ptr   Pointer to the bitmap
     */
    NONNULL
    static inline void operator delete (void *ptr)
    {
        Buddy::free (ptr);
    }
};

/*
 * Obtain bitmap word pointer for the specified selector
 *
 * @param s     Selector whose bitmap word pointer is being looked up
 * @param a     True if bitmap allocation is allowed, false otherwise
 * @param i     IO bitmap word pointer to hold the result
 * @return      True if successful, false otherwise
 */
bool Space_pio::walk (unsigned long s, bool a, uintptr_t *&i)
{
    // Read the bitmap pointer once without load tearing
    auto bmp = __atomic_load_n (&bitmap, __ATOMIC_RELAXED);

    // If the bitmap pointer is unset, we may have to allocate a new bitmap
    if (EXPECT_FALSE (!bmp)) {

        // Return if allocation is not allowed (e.g., during permission removal)
        if (!a)
            return false;

        // Allocate a new bitmap
        auto tmp = new Bitmap;

        // Return if allocation failed
        if (EXPECT_FALSE (!tmp))
            return false;

        // Try to plug our new bitmap in. If that fails, then someone beat us to it, so deallocate ours and continue with theirs.
        // Note: A compare_exchange failure changes bmp from nullptr to the existing value at bitmap
        if (EXPECT_FALSE (!__atomic_compare_exchange (&bitmap, &bmp, &tmp, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)))
            delete tmp;

        // Continue with our new bitmap upon success
        else
            static_cast<Pd *>(this)->Space_mem::update (SPC_LOCAL_IOP, Kmem::ptr_to_phys (bmp = tmp), 1, Paging::Permissions (Paging::W | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
    }

    // Set bitmap word pointer
    i = bmp->port + Bitmap::sel_to_word (s);

    return true;
}

/*
 * Lookup PIO bitmap for the specified selector
 *
 * @param s     Selector whose PIO bitmap bit is being looked up
 * @return      Permissions for the specified selector
 */
bool Space_pio::lookup (unsigned long s)
{
    // Bitmap word pointer and bitmask corresponding to selector
    uintptr_t *i, m = Bitmap::sel_to_mask (s);

    // Obtain bitmap word pointer for the specified selector
    if (!walk (s, false, i))
        return false;

    // Decode permissions
    return !(__atomic_load_n (i, __ATOMIC_RELAXED) & m);
}

/*
 * Update PIO bitmap for the specified selector
 *
 * @param s     Selector whose PIO bitmap bit is being updated
 * @param p     Permissions for the specified selector
 * @return      SUCCESS (successful) or INS_MEM (PIO bitmap allocation failure)
 */
Status Space_pio::update (unsigned long s, bool p)
{
    // Bitmap word pointer and bitmask corresponding to selector
    uintptr_t *i, m = Bitmap::sel_to_mask (s);

    // Obtain bitmap word pointer for the specified selector
    if (!walk (s, p, i))
        return p ? Status::INS_MEM : Status::SUCCESS;

    if (p)
        __atomic_fetch_and (i, ~m, __ATOMIC_SEQ_CST);
    else
        __atomic_fetch_or  (i,  m, __ATOMIC_SEQ_CST);

    return Status::SUCCESS;
}

/*
 * Destructor
 */
Space_pio::~Space_pio()
{
    if (bitmap)
        delete bitmap;
}
