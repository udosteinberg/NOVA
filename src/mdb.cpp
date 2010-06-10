/*
 * Mapping Database
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

#include "assert.h"
#include "initprio.h"
#include "lock_guard.h"
#include "mdb.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Mdb::cache (sizeof (Mdb), 16);

Spinlock Mdb::lock;

bool Mdb::insert_node (Mdb *p, mword a)
{
    Lock_guard <Spinlock> guard (lock);

    if (!p->alive())
        return false;

    if (!(node_attr = p->node_attr & a))
        return false;

    prev = p;
    next = p->next;
    dpth = p->dpth + 1;
    p->next = p->next->prev = this;

    return true;
}

void Mdb::demote_node (mword a)
{
    Lock_guard <Spinlock> guard (lock);

    node_attr &= ~a;
}

bool Mdb::remove_node()
{
    if (node_attr)
        return false;

    assert (dpth);

    Lock_guard <Spinlock> guard (lock);

    if (!alive())
        return false;

    if (next->dpth > dpth)
        return false;

    next->prev = prev;
    prev->next = next;

    return true;
}
