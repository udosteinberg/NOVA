/*
 * MSR Space
 *
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
#include "space_msr.hpp"

struct Space_msr::Bitmap
{
    static constexpr auto sels = BITN (13);
    static constexpr auto bits = 8 * sizeof (uintptr_t);

    uintptr_t r_lo[sels / bits] { ~0UL };   // RDMSR bitmap for lo MSRs
    uintptr_t r_hi[sels / bits] { ~0UL };   // RDMSR bitmap for hi MSRs
    uintptr_t w_lo[sels / bits] { ~0UL };   // WRMSR bitmap for lo MSRs
    uintptr_t w_hi[sels / bits] { ~0UL };   // WRMSR bitmap for hi MSRs

    static inline auto sel_to_mask (unsigned long s) { return BITN (s % bits); }
    static inline auto sel_to_word (unsigned long s) { return s % sels / bits; }

    /*
     * Allocate MSR bitmap
     *
     * @return      Pointer to the bitmap (allocation success) or nullptr (allocation failure)
     */
    NODISCARD ALWAYS_INLINE
    static inline void *operator new (size_t) noexcept
    {
        static_assert (sizeof (Bitmap) == PAGE_SIZE);
        return Buddy::alloc (0);
    }

    /*
     * Deallocate MSR bitmap
     *
     * @param ptr   Pointer to the bitmap
     */
    NONNULL ALWAYS_INLINE
    static inline void operator delete (void *ptr)
    {
        Buddy::free (ptr);
    }
};

/*
 * Obtain bitmap word pointers for the specified selector
 *
 * @param s     Selector whose bitmap word pointers are being looked up
 * @param a     True if bitmap allocation is allowed, false otherwise
 * @param r     RD bitmap word pointer to hold the result
 * @param w     WR bitmap word pointer to hold the result
 * @return      True if successful, false otherwise
 */
bool Space_msr::walk (unsigned long s, bool a, uintptr_t *&r, uintptr_t *&w)
{
    if (s > 0x1fff && (s < 0xc0000000 || s > 0xc0001fff))
        return false;

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
            bmp = tmp;
    }

    // Compute word number in bitmap
    auto n = Bitmap::sel_to_word (s);

    // Set bitmap word pointers
    if (s < bmp->sels) {
        r = bmp->r_lo + n;
        w = bmp->w_lo + n;
    } else {
        r = bmp->r_hi + n;
        w = bmp->w_hi + n;
    }

    return true;
}

/*
 * Lookup MSR bitmap for the specified selector
 *
 * @param s     Selector whose MSR bitmap bits are being looked up
 * @return      Permissions for the specified selector
 */
Paging::Permissions Space_msr::lookup (unsigned long s)
{
    // Bitmap word pointers and bitmask corresponding to selector
    uintptr_t *r, *w, m = Bitmap::sel_to_mask (s);

    // Obtain bitmap word pointers for the specified selector
    if (!walk (s, false, r, w))
        return Paging::Permissions (0);

    // Decode permissions
    return Paging::Permissions (!(__atomic_load_n (w, __ATOMIC_RELAXED) & m) * Paging::W |
                                !(__atomic_load_n (r, __ATOMIC_RELAXED) & m) * Paging::R);
}

/*
 * Update MSR bitmap for the specified selector
 *
 * @param s     Selector whose MSR bitmap bits are being updated
 * @param p     Permissions for the specified selector
 * @return      SUCCESS (successful) or INS_MEM (MSR bitmap allocation failure)
 */
Status Space_msr::update (unsigned long s, Paging::Permissions p)
{
    // Bitmap word pointers and bitmask corresponding to selector
    uintptr_t *r, *w, m = Bitmap::sel_to_mask (s);

    // Obtain bitmap word pointers for the specified selector
    if (!walk (s, p, r, w))
        return p ? Status::INS_MEM : Status::SUCCESS;

    // Update WRMSR bit
    if (p & Paging::W)
        __atomic_fetch_and (w, ~m, __ATOMIC_SEQ_CST);
    else
        __atomic_fetch_or  (w,  m, __ATOMIC_SEQ_CST);

    // Update RDMSR bit
    if (p & Paging::R)
        __atomic_fetch_and (r, ~m, __ATOMIC_SEQ_CST);
    else
        __atomic_fetch_or  (r,  m, __ATOMIC_SEQ_CST);

    return Status::SUCCESS;
}

/*
 * Destructor
 */
Space_msr::~Space_msr()
{
    if (bitmap)
        delete bitmap;
}
