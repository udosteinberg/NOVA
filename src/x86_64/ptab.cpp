/*
 * Page Table
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

#include "assert.hpp"
#include "ptab.hpp"
#include "ptab_tmpl.hpp"

/*
 * Pagetable consists of a tree of tables. Each table has size PAGE_SIZE, is indexed by
 * bpl (e.g. 9) bits of the input address (IAddr) and contains n=2^bpl (e.g. 512) slots.
 *
 * The number of levels (e.g. 4 in the example below) is hardware-dependent.
 *
 * Each slot in a table contains:
 *   - either 0 (invalid, representing a hole in the address space)
 *   - or the physical address (P) of a next-level table (PTable) + path attributes (hardware-dependent)
 *   - or the physical address (P) of a leaf page frame (PFrame) + page attributes (hardware-dependent)
 *
 * The output address (OAddr) is formed by combining the PFrame address (P) with offset bits (O) from IAddr.
 *
 *        ------------------------------------------------------------------------------------------------------------
 * IAddr | unused |  bpl bits (ddd)   |  bpl bits (ccc)   |  bpl bits (bbb)   |  bpl bits (aaa)   |  offset bits (O)  |
 *        -------------|-------------------|---------------------------------------|----------------------------|-----
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
 * @param n     Target level to walk down to
 * @param a     True if allocation of page tables is allowed, false otherwise
 * @param nc    True if the page table is non-coherent, false otherwise
 * @return      Pointer to the PTE (if exists), nullptr otherwise
 */
template <typename T, typename I, typename O, unsigned L, unsigned M, bool C>
typename Pagetable<T,I,O,L,M,C>::PTE *Pagetable<T,I,O,L,M,C>::walk (IAddr v, unsigned n, bool a, bool nc)
{
    auto l = lev; T pte;

    // Walk down the page tables from the root, computing the slot index at each level
    for (auto ptr = &root;; ptr = &pte->slot[(v >> (--l * bpl + PAGE_BITS)) % Table::entries]) {

        // Terminate the walk upon reaching the target level and return pointer to the PTE
        if (l == n)
            return ptr;

        // Atomically read the PTE from the slot
        for (pte = static_cast<T>(*ptr);;) {

            // A non-empty PTE must refer to either a large page or a page table
            assert (pte.is_empty() || pte.is_large (l) ^ pte.is_table (l));

            // Terminate the walk if allocation is not allowed (e.g., during page removal)
            if (pte.is_empty() && !a)
                return nullptr;

            // If the PTE is empty or refers to a large page, then we need a new page table for the next level
            if (pte.is_empty() || pte.is_large (l)) {

                // If the PTE is empty, then allocate an empty page table, otherwise splinter the large page
                OAddr p = pte.is_empty() ? 0 : pte.addr (l) | T::page_attr (l - 1, pte.page_pm(), pte.page_ca (l), pte.page_sh());
                OAddr s = pte.is_empty() ? 0 : T::page_size ((l - 1) * bpl);

                // Allocate a new page table
                auto tbl = new Table (p, s, nc);

                // Terminate the walk if allocation failed
                if (EXPECT_FALSE (!tbl))
                    return nullptr;

                // Construct a new PTE that refers to the new page table
                T tmp (Kmem::ptr_to_phys (tbl) | (l != L) * T::ptab_attr());

                // Try to install our new PTE into the slot
                // * Success: continue with our new page table
                // * Failure: someone beat us to it; deallocate our new page table and start over
                // Note: A compare_exchange failure changes pte to the existing value at ptr
                if (!ptr->compare_exchange (pte, tmp)) {
                    Table::operator delete (tbl, false);
                    continue;
                }

                // Ensure PTE coherence
                if (C && nc)
                    Cache::data_clean (ptr);

                pte = tmp;
            }

            // Proceed with the page table for the next level
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
 * @param ca    Reference to the cacheability attributes that are being returned
 * @param sh    Reference to the shareability attributes that are being returned
 * @return      Page permissions (0 for empty PTEs)
 */
template <typename T, typename I, typename O, unsigned L, unsigned M, bool C>
Paging::Permissions Pagetable<T,I,O,L,M,C>::lookup (IAddr v, OAddr &p, unsigned &o, Memattr::Cacheability &ca, Memattr::Shareability &sh) const
{
    auto l = lev; T pte;

    // Walk down the page tables from the root, computing the slot index at each level
    for (auto ptr = &root;; ptr = &pte->slot[(v >> (--l * bpl + PAGE_BITS)) % Table::entries]) {

        // Compute the page order for this level
        o = l * bpl;

        // Atomically read the PTE from the slot
        pte = static_cast<T>(*ptr);

        // If the PTE is empty, then there is a hole
        if (pte.is_empty())
            return Paging::Permissions (p = 0);

        // If the PTE refers to a page table, then proceed with the next level
        if (pte.is_table (l))
            continue;

        // Compute the physical address of the leaf page
        p = pte.addr (l) | (v & T::offs_mask (o));

        // Extract PTE attributes
        ca = pte.page_ca (l);
        sh = pte.page_sh();

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
 * @param ca    Cacheability attributes
 * @param sh    Shareability attributes
 * @param nc    True if the page table is non-coherent, false otherwise
 * @return      SUCCESS (successful) or INS_MEM (page table allocation failure)
 */
template <typename T, typename I, typename O, unsigned L, unsigned M, bool C>
Status Pagetable<T,I,O,L,M,C>::update (IAddr v, OAddr p, unsigned ord, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh, bool nc)
{
    // Both virtual and physical address must be order-aligned
    assert ((v & T::offs_mask (ord)) == 0);
    assert ((p & T::offs_mask (ord)) == 0);

    auto const o = min (ord, lim);
    auto const l = o / bpl;
    auto const n = BIT (o % bpl);
    auto const a = T::page_attr (l, pm, ca, sh);

    // Split operations that cross page-table boundaries into the largest possible size
    for (unsigned i = 0; i < BIT (ord - o); i++, v += BIT (o + PAGE_BITS), p += BIT (o + PAGE_BITS)) {

        // Get pointer to the first PTE
        auto ptr = walk (v, l, a, nc);

        // If there is no page table, then return error only if we wanted to allocate
        if (EXPECT_FALSE (!ptr))
            return a ? Status::INS_MEM : Status::SUCCESS;

        // Compute initial entry value and size increment
        OAddr e = a ? p | a : 0;
        OAddr s = a ? T::page_size (l * bpl) : 0;

        T old;

        // Iterate over all slots covering the range
        for (unsigned j = 0; j < n; j++, ptr++, e += s) {

            // Construct a new PTE
            T pte (e);

            // Atomically replace old with new PTE
            ptr->exchange (old, pte);

            // If the old PTE is empty, then there is nothing to clean up
            if (old.is_empty())
                continue;

            // If the old PTE refers to a page table, then deallocate it
            if (old.is_table (l))
                old->deallocate (l - 1);
        }

        // Ensure PTE coherence
        if (C && nc)
            Cache::data_clean (ptr, n * sizeof (*ptr));
    }

    return Status::SUCCESS;
}

/*
 * Deallocate a page table subtree
 *
 * @param l     Subtree level
 */
template <typename T, typename I, typename O, unsigned L, unsigned M, bool C>
void Pagetable<T,I,O,L,M,C>::Table::deallocate (unsigned l)
{
    if (!l)
        return;

    // Iterate over all slots
    for (unsigned i = 0; i < entries; i++) {

        // Atomically read the old PTE from the slot
        auto old = static_cast<T>(slot[i]);

        // If the old PTE is empty, then there is nothing to clean up
        if (old.is_empty())
            continue;

        // If the old PTE refers to a page table, then deallocate it
        if (old.is_table (l))
            old->deallocate (l - 1);
    }

    operator delete (this, false);
}
