/*
 * Object Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

INIT_PRIORITY (PRIO_SPACE_OBJ) ALIGNED (Kobject::alignment) Space_obj Space_obj::nova;

/*
 * The object space consists of a tree of Captables. A Captable has size PAGE_SIZE (0),
 * is indexed by bpl (e.g. 9) bits of the selector and contains n=2^bpl (e.g. 512) slots.
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
 *      --------------------------------------------------------------------
 * sel | unused |  bpl bits (ccc)   |  bpl bits (bbb)   |  bpl bits (aaa)   |
 *      -------------|-------------------|-------------------|--------------
 *                   |                   |                   |
 *                   |   -----------     |   -----------     |   -----------
 *                   |  | slot[n-1] |    |  | slot[n-1] |    |  | slot[n-1] |
 *                   |  |    ...    |    |  |    ...    |    |  |    ...    |
 *                   +->| slot[ccc] |-+  +->| slot[bbb] |-+  +->| slot[aaa] |-> Kobject
 *                      |    ...    |  \    |    ...    |  \    |    ...    |
 *       ------         | slot[001] |   \   | slot[001] |   \   | slot[001] |
 *      | root |------->| slot[000] |    +->| slot[000] |    +->| slot[000] |
 *       ------          -----------         -----------         -----------
 *
 *      Level 3         Level 2             Level 1             Level 0
 *      Captable *      Captable *[n]       Captable *[n]       Capability[n]
 */

struct Space_obj::Captable
{
    static constexpr auto entries { BIT (bpl) };

    Atomic<Captable *> slot[entries] { nullptr };

    /*
     * Allocate a Captable
     *
     * @return      Pointer to the Captable (allocation success) or nullptr (allocation failure)
     */
    [[nodiscard]] static void *operator new (size_t) noexcept
    {
        static_assert (sizeof (Captable) == PAGE_SIZE (0));
        return Buddy::alloc (0);
    }

    /*
     * Deallocate a Captable
     *
     * @param ptr   Pointer to the Captable
     */
    NONNULL static void operator delete (void *ptr)
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
            for (unsigned i { 0 }; i < entries; i++)
                if (slot[i])
                    slot[i]->deallocate (l - 1);

        delete this;
    }
};

/*
 * Destructor
 */
Space_obj::~Space_obj()
{
    if (root)
        root->deallocate (lev - 1);
}

/*
 * Walk capability tables and return pointer to the capability slot for the specified selector
 *
 * @param sel   Selector whose slot is being looked up
 * @param e     True if making entries, false if making holes
 * @return      Pointer to the capability slot (if exists) or ~0 (skippable hole) or nullptr (allocation failure)
 */
Atomic<Capability> *Space_obj::walk (unsigned long sel, bool e)
{
    auto l { lev }; Captable *cte;

    // Walk down the capability tables from the root, computing the slot index at each level
    for (auto ptr { &root };; ptr = &cte->slot[(sel >> --l * bpl) % Captable::entries]) {

        // Terminate the walk upon reaching the leaf level and return pointer to the capability slot
        if (!l)
            return reinterpret_cast<Atomic<Capability> *>(ptr);

        // If the capability table entry is empty, we may need a new capability table for the next level
        if (EXPECT_FALSE (!(cte = *ptr))) {

            // Terminate the walk for a skippable hole
            if (!e)
                return reinterpret_cast<Atomic<Capability> *>(~0UL);

            // Allocate a new capability table
            auto tbl { new Captable };

            // Terminate the walk if allocation failed
            if (EXPECT_FALSE (!tbl))
                return nullptr;

            // Try to install our new capability table into the supposedly empty slot
            // * Success: continue with our new capability table
            // * Failure: someone beat us to it; deallocate our new capability table and continue with theirs
            // Note: A compare_exchange failure changes cte from nullptr to the existing value at ptr
            if (EXPECT_TRUE (ptr->compare_exchange (cte, tbl)))
                cte = tbl;
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
    auto l { lev }; Captable *cte;

    // Walk down the capability tables from the root, computing the slot index at each level
    for (auto ptr { &root };; ptr = &cte->slot[(sel >> --l * bpl) % Captable::entries]) {

        // Return capability upon reaching the last existing or leaf level
        if (!(cte = *ptr) || !l)
            return Capability (reinterpret_cast<uintptr_t>(cte));
    }
}

/*
 * Update OBJ capability for the specified selector
 *
 * @param sel   Selector whose capability is being updated
 * @param cap   New capability for that selector
 * @param old   Old capability for that selector
 * @return      SUCCESS (successful) or MEM_CAP (allocation failure)
 */
Status Space_obj::update (unsigned long sel, Capability cap, Capability &old)
{
    // Get capability slot pointer
    auto const ptr { walk (sel, cap.prm()) };

    // Allocation failure
    if (EXPECT_FALSE (!ptr))
        return Status::MEM_CAP;

    // Skippable hole
    if (ptr == reinterpret_cast<Atomic<Capability> *>(~0UL))
        return Status::SUCCESS;

    // Replace old with new capability
    ptr->exchange (old, cap);

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
    auto const ptr { walk (sel, true) }; Capability old;

    // No slot, return error because we wanted to allocate
    if (EXPECT_FALSE (!ptr))
        return Status::MEM_CAP;

    // Try to install the new capability
    return ptr->compare_exchange (old, cap) ? Status::SUCCESS : Status::BAD_CAP;
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

    if (EXPECT_FALSE (sse > selectors || dse > selectors))
        return Status::BAD_PAR;

    auto sts { Status::SUCCESS };

    for (auto src { ssb }, dst { dsb }; src < sse; src++, dst++) {

        Capability cap { obj->lookup (src) }, old;

        auto const o { cap.obj() };
        auto const p { cap.prm() & pmm };

        // FIXME: Inc refcount for new capability object and dec refcount for old capability object
        if ((sts = update (dst, Capability (o, p), old)) != Status::SUCCESS)
            break;
    }

    return sts;
}
