/*
 * Virtual Memory Area
 *
 * Copyright (C) 2008-2010, Udo Steinberg <udo@hypervisor.org>
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

#include "stdio.h"
#include "vma.h"

Slab_cache Vma::cache (sizeof (Vma), 16);

// XXX: MP access to the VMA list of a PD should be protected by spinlock

Vma::Vma (Pd *p, mword b, mword o, mword t, mword a) : pd (p), base (b), order (o), type (t), attr (a)
{
    trace (TRACE_MAP, "--> VMA: PD:%p B:%#010lx O:%lu T:%#lx A:%#lx ", p, b, o, t, a);
}

bool Vma::insert (Vma *list, Vma *tree)
{
    // Check for colliding VMAs. If we find any, return an error.
    // If we are preempted during the (potentially long) list traversal, we
    // should roll back to the beginning of the IPC continuation. In that
    // case we don't change any kernel state.
    Vma *l;
    for (l = list->list_next; l != list; l = l->list_next) {

        if (collide (base, l->base, order, l->order))
            return false;

        if (l->base > base)
            break;
    }

    // Link in before l
    list_next = l;
    list_prev = l->list_prev;
    list_prev->list_next = list_next->list_prev = this;

    // Link in after tree
    tree_prev = tree;
    tree_next = tree->tree_next;
    tree_prev->tree_next = tree_next->tree_prev = this;

    return true;
}

void Vma::dump()
{
    for (Vma *vma = tree_next; vma->depth != depth; vma = vma->tree_next)
        trace (0, "D:%u B:%#010lx O:%lu A:%#lx PD:%p",
               vma->depth, vma->base, vma->order, vma->attr, vma->pd);
}
