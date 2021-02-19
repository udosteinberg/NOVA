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
 *   - LEAF = physical address (P) of the final page frame + page attributes (hardware-dependent)
 *   - PTAB = physical address (P) of the next-level table + path attributes (hardware-dependent)
 *   - DEAD = invalid, marking a slot of an unlinked table that must never be repopulated
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
 * @param inv   TLB invalidation scope
 * @return      Pointer to the PTE (if exists) or nullptr (otherwise)
 */
template<typename T, typename I, typename O> typename Ptab<T, I, O>::PTE *Ptab<T, I, O>::walk (IAddr v, unsigned t, bool &m, Tlb::Invalidate &inv)
{
    auto l { T::lev() }; T pte;

    // Walk down the page tables from the root, computing the slot index at each level
    for (auto ptr { &root };; ptr = pte.table() + T::lev_idx (--l, v)) {

        // Terminate the walk upon reaching the target level and return pointer to the PTE
        if (l == t)
            return ptr;

        // Atomically read the PTE from the slot
        pte = T { ptr->load() };

        // Determine the PTE type
        auto const type { pte.type (l) };

        // LEAF type cannot exist above maximum leaf level
        assert (type != Entry::Type::LEAF || l <= max_leaf_level);

        // Terminate the walk if the PTE is DEAD, which a concurrent dismantle made it, or if we need
        // a HOLE and it is already there, which a concurrent unmap may have put there and which we
        // cannot tell apart from a slot that was never mapped.
        if (type == Entry::Type::DEAD || (type == Entry::Type::HOLE && !m)) [[unlikely]]
            break;

        // If the PTE is HOLE or LEAF, then we need a new PTAB for the next level
        if (type != Entry::Type::PTAB) {

            // If the PTE is HOLE, then allocate an empty PTAB, otherwise splinter the LEAF
            bool  const h { type == Entry::Type::HOLE };
            auto  const n { l - 1 };
            OAddr const p { h ? 0 : pte.addr (l) | T::attr_leaf (n, pte.page_pm(), pte.page_ma (l)) };
            OAddr const s { h ? 0 : T::page_size (n * T::bpl) };

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
            // Note: A compare_exchange failure changes pte to the existing value at ptr
            if (ptr->compare_exchange (pte, tmp)) [[likely]] {

                // Ensure PTE observability
                Coherence::producer (T::noncoherent, ptr);

                // Determine if the change requires ALL invalidation
                if (tlb_inv_all (pte, tmp, l))
                    inv = Tlb::Invalidate::ALL;

                // Descend with our PTAB
                pte = tmp;

            } else {

                // Deallocate our PTAB
                operator delete (ptab, false);

                // Terminate the walk because our updates for this chunk are superseded by the winner's HOLE/LEAF/DEAD
                if (pte.type (l) != Entry::Type::PTAB)
                    break;

                // Descend with the winner's PTAB
            }
        }
    }

    // The caller can skip this chunk (no memory allocation failure)
    m = false;

    // No PTAB exists
    return nullptr;
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

        // HOLE/DEAD: Return no permissions
        if (type == Entry::Type::HOLE || type == Entry::Type::DEAD) {
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
 * @param pm    Page permissions (0 for unmapping PTEs)
 * @param ma    Memory attributes
 * @param inv   TLB invalidation scope
 * @return      SUCCESS (successful) or MEM_CAP (allocation failure)
 */
template<typename T, typename I, typename O> Status Ptab<T, I, O>::update (IAddr v, OAddr p, unsigned ord, Paging::Permissions pm, Memattr ma, Tlb::Invalidate &inv)
{
    // Both virtual and physical address must be order-aligned
    assert ((v & T::offs_mask (ord)) == 0);
    assert ((p & T::offs_mask (ord)) == 0);

    auto const l { min (ord / T::bpl, max_leaf_level) };
    auto const o { min (ord, T::lev_ord (l)) };
    auto const n { BIT (o - l * T::bpl) };
    auto const a { T::attr_leaf (l, pm, ma) };
    auto const s { a ? T::page_size (l * T::bpl) : 0 };

    // Split operations that cross page-table boundaries into the largest possible chunk
    for (unsigned i { 0 }; i < BITN (ord - o); i++, v += BITN (o + PAGE_BITS), p += BITN (o + PAGE_BITS)) {

        // True if making PTEs, false if breaking PTEs. Recomputed each iteration because walk can change it
        bool m { !!a };

        // Get pointer to the first PTE
        auto const ptr { walk (v, l, m, inv) };

        // Disambiguate "no PTE" case
        if (!ptr) [[unlikely]] {

            // PTAB allocation failure
            if (m) [[unlikely]]
                return Status::MEM_CAP;

            // Chunk can be skipped
            continue;
        }

        // Compute initial entry value
        OAddr e { a ? p | a : 0 };

        // Iterate over all slots covering the range, stop upon seeing a dead slot
        for (unsigned j { 0 }; j < n; j++, e += s)
            if (!replace (l, ptr[j], T { e }, inv)) [[unlikely]]
                break;

        // Ensure PTE observability
        Coherence::producer (T::noncoherent, ptr, sizeof (PTE) * n);
    }

    return Status::SUCCESS;
}
