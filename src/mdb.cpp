/*
 * Mapping Database
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

#include "lock_guard.h"
#include "mdb.h"

Slab_cache Map_node::cache (sizeof (Map_node), 16);

Map_node::Map_node (Pd *p, mword b, mword a) : prev (this), next (this), depth (0), pd (p), base (b), attr (a)
{
//    trace (TRACE_MAP, "--- MN:%p D:%02u PD:%p CD:%#lx", this, depth, pd, cd);
}

Map_node *Map_node::remove_child()
{
    Map_node *node = next;

    if (node->depth > depth) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        return node;
    }

    return 0;
}

void Map_node::add_child (Map_node *child)
{
    Lock_guard <Spinlock> guard (lock);

    // Make sure "this" node is not dead yet

    child->depth = depth + 1;
    child->next  = next;
    child->prev  = this;
    next = next->prev = child;
}
