/*
 * Page Table
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "cpu.hpp"
#include "ptab.hpp"
#include "ptab_tmp.hpp"

/*
 * Ptab consists of a tree of tables. Each table has size PAGE_SIZE (0), is indexed by
 * bpl (e.g. 9) bits of the input address (IAddr) and contains n=2^bpl (e.g. 512) slots.
 *
 * The number of levels (e.g. 4 in the example below) is hardware-dependent.
 *
 * The type of each page-table slot is one of the following:
 *   - HOLE = 0 (invalid, representing a hole in the address space)
 *   - PTAB = physical address (P) of the next-level table + path attributes (hardware-dependent)
 *   - LEAF = physical address (P) of the final page frame + page attributes (hardware-dependent)
 *
 * The output address (OAddr) is formed by combining the LEAF physical address (P) with offset bits (O) from IAddr.
 *
 *        ------------------------------------------------------------------------------------------------------------
 * IAddr | unused |  bpl bits (ddd)   |  bpl bits (ccc)   |  bpl bits (bbb)   |  bpl bits (aaa)   |  offset bits (O)  |
 *        -------------|-------------------|-------------------|-------------------|----------------------------|-----
 *                     |                   |                   |                   |                            |
 *        Level 4      |     Level 3       |     Level 2       |     Level 1       |     Level 0                |
 *                     |   -----------     |   -----------     |   -----------     |   -----------              |
 *                     |  | slot[n-1] |    |  | slot[n-1] |    |  | slot[n-1] |    |  | slot[n-1] |             |
 *                     |  |    ...    |    |  |    ...    |    |  |    ...    |    |  |    ...    |             |
 *                     +->| slot[ddd] |-+  +->| slot[ccc] |-+  +->| slot[bbb] |-+  +->| slot[aaa] |-+           |
 *                        |    ...    | |     |    ...    | |     |    ...    | |     |    ...    | |           |
 *        ------          | slot[001] | |     | slot[001] | |     | slot[001] | |     | slot[001] | |           |
 *       | root |-------->| slot[000] | ?---->| slot[000] | ?---->| slot[000] | ?---->| slot[000] | |           |
 *        ------           -----------  |      -----------  |      -----------  |      -----------  |           v
 *                                      |                   |                   |                   +--> P[X:12]+O[11:0] = OAddr (4K)
 *                                      |                   |                   +-----aaa part of O----> P[X:21]+O[20:0] = OAddr (2M)
 *                                      |                   +----------------------bbbaaa part of O----> P[X:30]+O[29:0] = OAddr (1G)
 *                                      +---------------------------------------cccbbbaaa part of O----> P[X:39]+O[38:0] = OAddr (512G)
 */

/*
 * Walk page tables and return pointer to the PTE for the specified virtual address
 *
 * @param v     Virtual address whose PTE is being looked up
 * @param t     Target level to walk down to
 * @param m     True if making PTEs, false if breaking PTEs
 * @param inv   True if TLB invalidation is required
 * @return      Pointer to the PTE (if exists) or nullptr (otherwise)
 */
template<typename T, typename I, typename O> typename Ptab<T, I, O>::PTE *Ptab<T, I, O>::walk (IAddr v, unsigned t, bool &m, bool &inv)
{
    auto l { T::lev() }; T pte;

    // Walk down the page tables from the root, computing the slot index at each level
    for (auto ptr { &root };; ptr = pte.table() + T::lev_idx (--l, v)) {

        // Terminate the walk upon reaching the target level and return pointer to the PTE
        if (l == t)
            return ptr;

        // Atomically read the PTE from the slot
        for (pte = T { ptr->load() };;) {

            // Determine the PTE type
            auto const type { pte.type (l) };

            // LEAF type cannot exist above maximum leaf level
            assert (type != Entry::Type::LEAF || l <= max_leaf_level);

            // Terminate the walk if we need a HOLE and it is already there
            if (type == Entry::Type::HOLE && !m)
                return nullptr;

            // If the PTE is HOLE or LEAF, then we need a new PTAB for the next level
            if (type != Entry::Type::PTAB) {

                // If the PTE is HOLE, then allocate an empty PTAB, otherwise splinter the LEAF
                auto  const n { l - 1 };
                OAddr const p { (type == Entry::Type::LEAF) * (pte.addr (l) | T::attr_leaf (n, pte.page_pm(), pte.page_ma (l))) };
                OAddr const s { (type == Entry::Type::LEAF) * T::page_size (n * T::bpl) };

                // Allocate and initialize a new PTAB
                auto const ptab { Ptab::create (n, p, s) };

                // Terminate the walk if allocation failed (and inform caller of memory allocation failure)
                if (!ptab) [[unlikely]] {
                    m = true;
                    return nullptr;
                }

                // Construct a PTE that refers to the new PTAB (no attributes for root slot)
                T tmp { Kmem::ptr_to_phys (ptab) | (l < T::lev() ? T::attr_ptab (l) : 0) };

                // Try to install the PTE into the slot
                // * Success: continue with our new page table
                // * Failure: someone beat us to it; deallocate our new page table and start over
                // Note: A compare_exchange failure changes pte to the existing value at ptr
                if (!ptr->compare_exchange (pte, tmp)) {
                    operator delete (ptab, false);
                    continue;
                }

                // Ensure PTE observability
                Coherence::producer (T::noncoherent, ptr);

                // Determine if the change requires TLB invalidation
                inv |= tlb_inv (pte, tmp, l);

                pte = tmp;
            }

            // Proceed with PTAB for the next level
            break;
        }
    }
}

/*
 * Lookup PTE for the specified virtual address
 *
 * @param v     Virtual address whose PTE is being looked up
 * @param p     Reference to the physical address that is being returned
 * @param o     Reference to the page order that is being returned
 * @param a     Reference to the memory attributes that are being returned
 * @return      Page permissions (0 for HOLE)
 */
template<typename T, typename I, typename O> Paging::Permissions Ptab<T, I, O>::lookup (IAddr v, OAddr &p, unsigned &o, Memattr &a) const
{
    auto l { T::lev() }; T pte;

    // Walk down the page tables from the root, computing the slot index at each level
    for (auto ptr { &root };; ptr = pte.table() + T::lev_idx (--l, v)) {

        // Atomically read the PTE from the slot
        pte = T { ptr->load() };

        // Determine the PTE type
        auto const type { pte.type (l) };

        // PTAB: Proceed with the next level
        if (type == Entry::Type::PTAB)
            continue;

        // Compute the page order at this level
        o = l * T::bpl;

        // HOLE: Return no permissions
        if (type == Entry::Type::HOLE) {
            p = 0;
            a = Memattr::ram();
            return Paging::Permissions::NONE;
        }

        // LEAF: Compute the output address
        p = pte.addr (l) | (v & T::offs_mask (o));

        // Extract PTE attributes
        a = pte.page_ma (l);

        // Extract PTE permissions
        return pte.page_pm();
    }
}

/*
 * Update PTEs for the specified virtual address range
 *
 * @param v     Virtual base address of the range
 * @param p     Physical base address of the range
 * @param ord   Page order (2^ord pages) of the range
 * @param pm    Page permissions (0 for zapping PTEs)
 * @param ma    Memory attributes
 * @param inv   True if TLB invalidation is required
 * @return      SUCCESS (successful) or MEM_CAP (allocation failure)
 */
template<typename T, typename I, typename O> Status Ptab<T, I, O>::update (IAddr v, OAddr p, unsigned ord, Paging::Permissions pm, Memattr ma, bool &inv)
{
    // Both virtual and physical address must be order-aligned
    assert ((v & T::offs_mask (ord)) == 0);
    assert ((p & T::offs_mask (ord)) == 0);

    auto const l { min (ord / T::bpl, max_leaf_level) };
    auto const o { min (ord, T::lev_ord (l)) };
    auto const n { BIT (o - l * T::bpl) };
    auto const a { T::attr_leaf (l, pm, ma) };

    // True if making PTEs, false if breaking PTEs
    bool m { !!a };

    // Split operations that cross page-table boundaries into the largest possible size
    for (unsigned i { 0 }; i < BITN (ord - o); i++, v += BITN (o + PAGE_BITS), p += BITN (o + PAGE_BITS)) {

        // Get pointer to the first PTE
        auto const ptr { walk (v, l, m, inv) };

        // Disambiguate "no PTE" case
        if (!ptr) [[unlikely]] {

            // PTAB allocation failure
            if (m) [[unlikely]]
                return Status::MEM_CAP;

            // HOLE that can be skipped
            continue;
        }

        // Compute initial entry value and size increment
        OAddr e { a ? p | a : 0 };
        OAddr s { a ? T::page_size (l * T::bpl) : 0 };

        // Iterate over all slots covering the range
        for (unsigned j { 0 }; j < n; j++, e += s)
            inv |= replace (l, ptr[j], T { e }, true);

        // Ensure PTE observability
        Coherence::producer (T::noncoherent, ptr, sizeof (PTE) * n);
    }

    return Status::SUCCESS;
}
