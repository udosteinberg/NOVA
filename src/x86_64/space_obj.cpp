/*
 * Object Space
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
#include "space_obj.hpp"

/*
 * The object space consists of a tree of Captables. A Captable has size PAGE_SIZE, is
 * indexed by bpl (e.g. 9) bits of the selector and contains n=2^bpl (e.g. 512) slots.
 *
 * The number of levels (e.g. 3 in the example below) is configurable.
 *
 * Leaf Captables (level 0) store Capabilities:
 *   - either Kobject * (Object Capability)
 *   - or nullptr (Null Capability)
 * Non-leaf Captables (level > 0) store pointers to the next level:
 *   - either Captable * (next level exists)
 *   - or nullptr (next level does not yet exist)
 * The root is a single Captable * and is not indexed.
 *
 *      -------------------------------------------------------------------
 * sel |   0   |  bpl bits (kkk)   |  bpl bits (jjj)   |  bpl bits (iii)   |
 *      -------------------------------------------------------------------
 *                  |                   |                   |
 *                  |   -----------     |   -----------     |   -----------
 *                  |  | slot[n-1] |    |  | slot[n-1] |    |  | slot[n-1] |
 *                  |  |    ...    |    |  |    ...    |    |  |    ...    |
 *                  +->| slot[kkk] |-+  +->| slot[jjj] |-+  +->| slot[iii] |-> Kobject
 *                     |    ...    | |     |    ...    | |     |    ...    |
 *      ------         | slot[001] | |     | slot[001] | |     | slot[001] |
 *     | root |------->| slot[000] | +---->| slot[000] | +---->| slot[000] |
 *      ------          -----------         -----------         -----------
 *
 *     Level 3         Level 2             Level 1             Level 0
 *     Captable *      Captable *[n]       Captable *[n]       Capability[n]
 */

struct Space_obj::Captable
{
    static constexpr auto entries = BIT (bpl);

    Captable *slot[entries] { nullptr };

    /*
     * Allocate a Captable
     *
     * @return      Pointer to the Captable (allocation success) or nullptr (allocation failure)
     */
    NODISCARD
    static inline void *operator new (size_t) noexcept
    {
        return Buddy::alloc (0);
    }

    /*
     * Deallocate a Captable
     *
     * @param ptr   Pointer to the Captable
     */
    NONNULL
    static inline void operator delete (void *ptr)
    {
        Buddy::free (ptr);
    }

    /*
     * Deallocate a Captable subtree
     *
     * @param l     Subtree level
     */
    inline void deallocate (unsigned l)
    {
        if (l)
            for (unsigned i = 0; i < entries; i++)
                if (slot[i])
                    slot[i]->deallocate (l - 1);

        delete this;
    }
};

/*
 * Walk capability tables and return pointer to the capability slot for the specified selector
 *
 * @param sel   Selector whose slot is being looked up
 * @param a     True if allocation of capability tables is allowed, false otherwise
 * @return      Pointer to the capability slot (if exists), nullptr otherwise
 */
Capability *Space_obj::walk (unsigned long sel, bool a)
{
    auto l = lev;

    // Walk down the capability tables from the root, computing the slot index at each level
    for (Captable **ptr = &root, *cte;; ptr = &cte->slot[(sel >> --l * bpl) % Captable::entries]) {

        // Terminate the walk upon reaching the leaf level and return pointer to the capability slot
        if (!l)
            return reinterpret_cast<Capability *>(ptr);

        // At a non-leaf level, read the capability table entry from slot once without load tearing
        __atomic_load (ptr, &cte, __ATOMIC_RELAXED);

        // If the capability table entry is empty, we may have to allocate a new capability table for the next level
        if (EXPECT_FALSE (!cte)) {

            // Terminate the walk if allocation is not allowed (e.g., during capability removal)
            if (!a)
                return nullptr;

            // Allocate a new capability table
            auto tbl = new Captable;

            // Terminate the walk if allocation failed
            if (EXPECT_FALSE (!tbl))
                return nullptr;

            // Try to plug our new capability table into the empty slot. If that fails, then someone beat us to it, so deallocate ours and continue with theirs.
            // Note: A compare_exchange failure changes cte from nullptr to the existing value at ptr
            if (EXPECT_FALSE (!__atomic_compare_exchange (ptr, &cte, &tbl, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)))
                delete tbl;

            // Continue with our new capability table upon success
            else
                cte = tbl;
        }

        // Go to the capability table for the next level
    }
}

/*
 * Lookup capability for the specified selector
 *
 * @param sel   Selector whose capability is being looked up
 * @return      Object Capability (if slot is non-empty) or Null Capability (otherwise)
 */
Capability Space_obj::lookup (unsigned long sel) const
{
    auto l = lev;

    // Walk down the capability tables from the root, computing the slot index at each level
    for (Captable * const *ptr = &root, *cte;; ptr = &cte->slot[(sel >> --l * bpl) % Captable::entries]) {

        // Read the slot once without load tearing
        __atomic_load (ptr, &cte, __ATOMIC_RELAXED);

        // Return capability upon reaching the leaf or last existing level
        if (!l || !cte)
            return Capability (reinterpret_cast<uintptr_t>(cte));
    }
}

/*
 * Update capability for the specified selector
 *
 * @param sel   Selector whose capability is being updated
 * @param cap   New capability for that selector
 * @param old   Old capability for that selector
 * @return      SUCCESS (successful) or INS_MEM (capability table allocation failure)
 */
Status Space_obj::update (unsigned long sel, Capability cap, Capability &old)
{
    // Allocate cap tables only if not null capability
    bool a = cap.prm();

    // Get capability slot pointer
    Capability *ptr = walk (sel, a);

    // No slot, return error only if we wanted to allocate
    if (EXPECT_FALSE (!ptr))
        return a ? Status::INS_MEM : Status::SUCCESS;

    // Replace old with new capability
    __atomic_exchange (ptr, &cap, &old, __ATOMIC_SEQ_CST);

    return Status::SUCCESS;
}

/*
 * Insert capability for the specified selector if slot is empty
 *
 * @param sel   Selector whose capability is being inserted
 * @param cap   New capability for that selector (must not be a null capability)
 * @return      SUCCESS (successful) or INS_MEM (capability table allocation failure) or BAD_CAP (slot not empty)
 */
Status Space_obj::insert (unsigned long sel, Capability cap)
{
    // Get capability slot pointer. Allocate based on assumption that cap is not a null capability
    Capability *ptr = walk (sel, true), old;

    // No slot, return error because we wanted to allocate
    if (EXPECT_FALSE (!ptr))
        return Status::INS_MEM;

    // Try to install the new capability
    return __atomic_compare_exchange (ptr, &old, &cap, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? Status::SUCCESS : Status::BAD_CAP;
}

/*
 * Destructor
 */
Space_obj::~Space_obj()
{
    if (root)
        root->deallocate (lev - 1);
}
