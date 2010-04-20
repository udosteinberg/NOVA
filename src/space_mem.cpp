/*
 * Memory Space
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "bits.h"
#include "mtrr.h"
#include "pd.h"

unsigned Space_mem::did_ctr;

void Space_mem::init (unsigned cpu)
{
    if (!percpu[cpu]) {

        // Create ptab for this CPU
        percpu[cpu] = new Ptab;

        // Sync cpu-local range
        percpu[cpu]->sync_from (Pd::kern.cpu_ptab (cpu), LOCAL_SADDR);

        // Sync kernel code and data
        percpu[cpu]->sync_master_range (LINK_ADDR, LOCAL_SADDR);

        trace (TRACE_MEMORY, "PD:%p PTAB[%u]:%#lx", static_cast<Pd *>(this), cpu, Buddy::ptr_to_phys (percpu[cpu]));
    }
}

void Space_mem::insert (Mdb *mdb, Paddr phys)
{
    Space_mem *s = mdb->node_pd;
    assert (s && s != &Pd::kern);

    unsigned o = mdb->node_order - PAGE_BITS;

    if (s->dpt) {
        unsigned ord = min (o, Dpt::ord);
        for (unsigned i = 0; i < 1u << (o - ord); i++)
            s->dpt->insert (mdb->node_base + i * (1UL << (Dpt::ord + PAGE_BITS)), ord, phys + i * (1UL << (Dpt::ord + PAGE_BITS)), mdb->node_attr);
    }

    if (s->ept) {
        unsigned ord = min (o, Ept::ord);
        for (unsigned i = 0; i < 1u << (o - ord); i++)
            s->ept->insert (mdb->node_base + i * (1UL << (Ept::ord + PAGE_BITS)), ord, phys + i * (1UL << (Ept::ord + PAGE_BITS)), mdb->node_attr, mdb->node_type);
    }

    Ptab::Attribute a = Ptab::Attribute (Ptab::ATTR_USER |
                 (mdb->node_attr & 0x4 ? Ptab::ATTR_NONE     : Ptab::ATTR_NOEXEC) |
                 (mdb->node_attr & 0x2 ? Ptab::ATTR_WRITABLE : Ptab::ATTR_NONE));

    // Whoever owns a VMA struct in the VMA list owns the respective PT slots
    s->mst->insert (mdb->node_base, o, a, phys);
}

void Space_mem::insert_root (mword base, size_t size, mword attr)
{
    for (size_t frag; size; size -= frag) {

        unsigned type = Mtrr::memtype (base);

        for (frag = 0; frag < size; frag += PAGE_SIZE)
            if (Mtrr::memtype (base + frag) != type)
                break;

        size_t s = frag;

        for (unsigned o; s; s -= 1UL << o, base += 1UL << o)
            insert_node (new Mdb (&Pd::kern, base, o = max_order (base, s), attr, type));
    }
}

bool Space_mem::insert_utcb (mword b)
{
    if (!b)
        return true;

    Mdb *mdb = new Mdb (&Pd::kern, b, PAGE_BITS);

    if (insert_node (mdb))
        return true;

    delete mdb;

    return false;
}
