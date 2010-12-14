/*
 * Generic Space
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#pragma once

#include "bits.h"
#include "compiler.h"
#include "lock_guard.h"
#include "mdb.h"
#include "spinlock.h"

class Space
{
    private:
        Spinlock    lock;
        Avl *       tree;

    public:
        Space() : tree (0) {}

        Mdb *lookup_node (mword idx, bool next = false)
        {
            Lock_guard <Spinlock> guard (lock);
            return Mdb::lookup (tree, idx, next);
        }

        bool insert_node (Mdb *node)
        {
            Lock_guard <Spinlock> guard (lock);
            return Mdb::insert (&tree, node);
        }

        bool remove_node (Mdb *node)
        {
            Lock_guard <Spinlock> guard (lock);
            return Mdb::remove (&tree, node);
        }

        void addreg (mword addr, size_t size, mword attr, mword type = 0)
        {
            Lock_guard <Spinlock> guard (lock);

            for (mword o; size; size -= 1UL << o, addr += 1UL << o)
                Mdb::insert (&tree, new Mdb (0, addr, addr, (o = max_order (addr, size)), attr, type));
        }

        void delreg (mword addr)
        {
            Mdb *node;

            {   Lock_guard <Spinlock> guard (lock);

                if (!(node = Mdb::lookup (tree, addr >>= PAGE_BITS, false)))
                    return;

                Mdb::remove (&tree, node);
            }

            mword next = addr + 1, base = node->node_base, last = base + (1UL << node->node_order);

            addreg (base, addr - base, node->node_attr, node->node_type);
            addreg (next, last - next, node->node_attr, node->node_type);

            delete node;
        }
};
