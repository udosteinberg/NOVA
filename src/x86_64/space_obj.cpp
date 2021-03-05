/*
 * Object Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "space_obj.hpp"

INIT_PRIORITY (PRIO_SPACE_OBJ) ALIGNED (Kobject::alignment) Space_obj Space_obj::nova;

/*
 * An object space consists of a tree of tables. Each table has size = PAGE_SIZE (0), is
 * indexed by bpl (e.g. 9) bits of the selector and contains n = 2^bpl (e.g. 512) slots.
 *
 * The number of levels (e.g. 3 in the example below) is software-configurable.
 *
 * The root is a single entry and is not indexed.
 * Level > 0: each uintptr_t entry stores a Captable*:
 *   - either 0 (next level not yet allocated)
 *   - or non-null (pointer to next level)
 * Level = 0: each uintptr_t entry stores a Capability:
 *   - either 0 (null capability)
 *   - or non-null (object capability)
 *
 *      --------------------------------------------------------------------
 * sel | unused |  bpl bits (ccc)   |  bpl bits (bbb)   |  bpl bits (aaa)   |
 *      -------------|-------------------|-------------------|--------------
 *                   |                   |                   |
 *       Level 3     |     Level 2       |     Level 1       |     Level 0
 *                   |   -----------     |   -----------     |   -----------
 *                   |  | slot[n-1] |    |  | slot[n-1] |    |  | slot[n-1] |
 *                   |  |    ...    |    |  |    ...    |    |  |    ...    |
 *                   +->| slot[ccc] |-+  +->| slot[bbb] |-+  +->| slot[aaa] |-> Capability
 *                      |    ...    | |     |    ...    | |     |    ...    |
 *       ------         | slot[001] | |     | slot[001] | |     | slot[001] |
 *      | root |------->| slot[000] | +---->| slot[000] | +---->| slot[000] |
 *       ------          -----------         -----------         -----------
 */

/*
 * Walk capability tables and return pointer to the capability slot for the specified selector
 *
 * @param sel   Selector whose slot is being looked up
 * @param a     True if table allocation is desired, false otherwise
 * @return      Pointer to the capability slot (if exists) or nullptr (otherwise)
 */
Space_obj::entry_t *Space_obj::walk (unsigned long sel, bool a)
{
    auto l { lev }; uintptr_t cte;

    // Walk down the capability tables from the root, computing the slot index at each level
    for (auto ptr { &root };; ptr = &table (cte)->slot[(sel >> --l * bpl) % Captable::entries]) {

        // Terminate the walk upon reaching the leaf level and return pointer to the capability slot
        if (!l)
            return ptr;

        // If the capability table entry is empty, we may need a new capability table for the next level
        if (!(cte = *ptr)) [[unlikely]] {

            // Terminate the walk if we encountered and desired a hole
            if (!a) [[unlikely]]
                return nullptr;

            // Allocate a new capability table
            auto const tbl { new Captable };

            // Terminate the walk if allocation failed
            if (!tbl) [[unlikely]]
                return nullptr;

            // Construct a CTE that refers to the new capability table
            auto const tmp { reinterpret_cast<uintptr_t>(tbl) };

            // Try to install the CTE into the empty slot
            // * Success: continue with our new capability table
            // * Failure: someone beat us to it; deallocate our new capability table and continue with theirs
            // Note: A compare_exchange failure changes cte to the existing value at ptr
            if (ptr->compare_exchange_n (cte, tmp)) [[likely]]
                cte = tmp;
            else
                delete tbl;
        }

        // Proceed with the capability table for the next level
    }
}

/*
 * Lookup OBJ capability for the specified selector
 *
 * @param sel   Selector whose capability is being looked up
 * @return      Object Capability (if slot is non-empty) or Null Capability (otherwise)
 */
Capability Space_obj::lookup (unsigned long sel) const
{
    auto l { lev }; uintptr_t cte;

    // Walk down the capability tables from the root, computing the slot index at each level
    for (auto ptr { &root };; ptr = &table (cte)->slot[(sel >> --l * bpl) % Captable::entries]) {

        // Return capability upon reaching the last existing or leaf level
        if (!(cte = *ptr) || !l)
            return Capability { cte };
    }
}

/*
 * Update OBJ capability for the specified selector
 *
 * @param sel   Selector whose capability is being updated
 * @param cap   New capability for that selector
 * @return      SUCCESS (successful) or MEM_CAP (allocation failure)
 */
Status Space_obj::update (unsigned long sel, Capability cap)
{
    bool const a { !!cap.prm() };

    // Get capability slot pointer
    auto const ptr { walk (sel, a) };

    // Disambiguate allocation failure from hole
    if (!ptr) [[unlikely]]
        return a ? Status::MEM_CAP : Status::SUCCESS;

    // Try to acquire a reference on the capability object and otherwise replace with null capability
    auto const old { ptr->exchange_n (cap.acquire() ? cap.val : 0) };

    // Release reference on the replaced capability object
    Capability { old }.release();

    return Status::SUCCESS;
}

/*
 * Insert OBJ capability for the specified selector if slot is empty
 *
 * @param sel   Selector whose capability is being inserted
 * @param cap   New capability for that selector (must not be a null capability)
 * @return      SUCCESS (successful) or MEM_CAP (allocation failure) or BAD_CAP (slot not empty)
 */
Status Space_obj::insert (unsigned long sel, Capability cap)
{
    // Get capability slot pointer. Allocate based on assumption that cap is not a null capability
    auto const ptr { walk (sel, true) };

    // No slot, return error because we wanted to allocate
    if (!ptr) [[unlikely]]
        return Status::MEM_CAP;

    // Must not be a null capability
    assert (cap.obj());

    // Acquire a reference on the capability object
    cap.publish();

    // Try to install the capability
    uintptr_t old {};
    if (ptr->compare_exchange_n (old, cap.val)) [[likely]]
        return Status::SUCCESS;

    // Release reference on the capability object
    cap.retract();

    return Status::BAD_CAP;
}

/*
 * Delegate OBJ capability range
 *
 * @param obj   Source OBJ space
 * @param ssb   Selector base (source)
 * @param dsb   Selector base (destination)
 * @param ord   Selector order (2^ord selectors)
 * @param pmm   Permission mask
 * @return      SUCCESS (successful) or MEM_CAP (allocation failure) or BAD_PAR (bad parameter)
 */
Status Space_obj::delegate (Space_obj const *obj, unsigned long const ssb, unsigned long const dsb, unsigned const ord, unsigned const pmm)
{
    auto const sse { ssb + BITN (ord) }, dse { dsb + BITN (ord) };

    if (sse > selectors || dse > selectors) [[unlikely]]
        return Status::BAD_PAR;

    auto sts { Status::SUCCESS };

    for (auto src { ssb }, dst { dsb }; src < sse; src++, dst++) {

        auto const cap { obj->lookup (src) };

        if ((sts = update (dst, Capability { cap.obj(), cap.prm() & pmm })) != Status::SUCCESS)
            break;
    }

    return sts;
}
