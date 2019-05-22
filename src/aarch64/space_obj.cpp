/*
 * Object Space
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

#include "buddy.hpp"
#include "space_obj.hpp"

/*
 * Overall Structure:
 *
 * The object space consists of a tree of PAGE_SIZE tables. Each table is indexed
 * by bpl (e.g. 9) bits of the selector and contains n=2^bpl (e.g. 512) slots.
 *
 * The number of levels (e.g. 3 in the example below) is configurable.
 * Leaf tables (level 0) store capabilities: either nullptr or kobject ptr
 * Non-leaf tables (level > 0) store pointers to the next level: either nullptr or captable ptr
 * The root is treated as a single-slot table and is not indexed.
 *
 *      -------------------------------------------------------------------
 * sel |   0   |  bpl bits (kkk)   |  bpl bits (jjj)   |  bpl bits (iii)   |
 *      -------------------------------------------------------------------
 *                  |                   |                   |
 *                  |   -----------     |   -----------     |   -----------
 *                  |  | slot[n-1] |    |  | slot[n-1] |    |  | slot[n-1] |
 *                  |  |    ...    |    |  |    ...    |    |  |    ...    |
 *                  +->| slot[kkk] |-+  +->| slot[jjj] |-+  +->| slot[iii] |
 *                     |    ...    | |     |    ...    | |     |    ...    |
 *      ------         | slot[001] | |     | slot[001] | |     | slot[001] |
 *     | root |------->| slot[000] | +---->| slot[000] | +---->| slot[000] |
 *      ------          -----------         -----------         -----------
 *      level 3           level 2             level 1             level 0
 *
 * slots contain:       captable ptr        captable ptr        capability
 *
 */

/*
 * Walk capability tables and return the slot for the specified selector
 *
 * @param sel   Selector whose slot is being looked up
 * @param a     True if allocation of capability tables is allowed, false otherwise
 * @return      Pointer to the capability slot (if exists), nullptr otherwise
 */
Capability *Space_obj::walk (unsigned long sel, bool a)
{
    auto l = lev;

    // Walk down the capability tables from the root, computing the slot index at each level
    for (Capability **e = &root, *cte;; e = reinterpret_cast<decltype (e)>(cte) + (sel >> (--l * bpl) & (BIT (bpl) - 1))) {

        // Terminate the walk upon reaching the leaf level and return the slot pointer
        if (!l)
            return reinterpret_cast<Capability *>(e);

        // Read the non-leaf slot once without load tearing
        __atomic_load (e, &cte, __ATOMIC_RELAXED);

        // If the non-leaf slot is empty, we may have to allocate a new capability table for the next level
        if (EXPECT_FALSE (!cte)) {

            // Terminate the walk if allocation is not allowed (e.g., during capability removal)
            if (!a)
                return nullptr;

            // Allocate a new capability table
            auto tmp = static_cast<Capability *>(Buddy::alloc (0, Buddy::Fill::BITS0));

            // Terminate the walk if allocation failed
            if (EXPECT_FALSE (!tmp))
                return nullptr;

            // Try to plug our new capability table into the empty slot. If that fails, then someone beat us to it, so deallocate ours and continue with theirs.
            // Note: A compare_exchange failure changes cte from nullptr to the existing value at e
            if (EXPECT_FALSE (!__atomic_compare_exchange (e, &cte, &tmp, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)))
                Buddy::free (tmp);

            // Continue with our new capability table upon success
            else
                cte = tmp;
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

    // Walk down the capability tables from the root pointer, computing the slot index at each level
    for (Capability * const *e = &root, *cte;; e = reinterpret_cast<decltype (e)>(cte) + (sel >> (--l * bpl) & (BIT (bpl) - 1))) {

        // Read the slot once without load tearing
        __atomic_load (e, &cte, __ATOMIC_RELAXED);

        // Return capability upon reaching the leaf or last level
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

    // Get slot pointer
    Capability *cte = walk (sel, a);

    // No slot, return error only if we wanted to allocate
    if (EXPECT_FALSE (!cte))
        return a ? Status::INS_MEM : Status::SUCCESS;

    // Replace old with new capability
    __atomic_exchange (cte, &cap, &old, __ATOMIC_SEQ_CST);

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
    // Get slot pointer. Allocate based on assumption that cap is not a null capability
    Capability *cte = walk (sel, true), old (0);

    // No slot, return error because we wanted to allocate
    if (EXPECT_FALSE (!cte))
        return Status::INS_MEM;

    // Try to install the new capability
    return __atomic_compare_exchange (cte, &old, &cap, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? Status::SUCCESS : Status::BAD_CAP;
}
