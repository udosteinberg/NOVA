/*
 * Virtual Memory Area
 *
 * Copyright (C) 2008, Udo Steinberg <udo@hypervisor.org>
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

#include "util.h"
#include "vma.h"

Slab_cache Vma::cache (sizeof (Vma), 16);

// XXX: MP access to the VMA list of a PD should be protected by spinlock

Vma::Vma (Vma *list, Vma *tree, unsigned d, Pd *p, mword b, mword o, mword t, mword a) : depth (d), pd (p), base (b), order (o), type (t), attr (a)
{
    // Link in before list
    list_next = list;
    list_prev = list->list_prev;
    list_prev->list_next = list_next->list_prev = this;

    // Link in after tree
    tree_prev = tree;
    tree_next = tree->tree_next;
    tree_prev->tree_next = tree_next->tree_prev = this;

    trace (TRACE_MAP, "--> VMA: D:%u PD:%p B:%#010lx O:%lu T:%#lx A:%#lx ", d, p, b, o, t, a);
}

Vma *Vma::create_child (Vma *list, Pd *p, mword b, mword o, mword t, mword a)
{
    // Check for colliding VMAs. If we find any, return an error. In the
    // future we might want to do an overmap/flush operation instead.
    // If we are preempted during the (potentially long) list traversal, we
    // should roll back to the beginning of the IPC continuation. In that
    // case we don't change any kernel state.
    Vma *v;
    for (v = list->list_next; v != list; v = v->list_next) {

        if (collide (b, v->base, o, v->order))
            return 0;

        if (v->base > b)
            break;
    }

    // Insertion of the VMA into the VMA list and the MDB tree must be
    // done atomically (same for removal). A concurrent unmap operation
    // must see MDB tree and VMA list in a consistent state.
    Vma *vma = new Vma (v, this, depth + 1, p, b, o, t, a);

    return vma;
}

void Vma::dump()
{
    for (Vma *vma = tree_next; vma->depth != depth; vma = vma->tree_next)
        trace (0, "D:%u B:%#010lx O:%lu A:%#lx PD:%p",
               vma->depth, vma->base, vma->order, vma->attr, vma->pd);
}
