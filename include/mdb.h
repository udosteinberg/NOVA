/*
 * Mapping Database
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

#include "avl.h"
#include "compiler.h"
#include "slab.h"
#include "spinlock.h"
#include "util.h"

class Pd;

class Mdb : public Avl
{
    private:
        static Slab_cache   cache;
        static Spinlock     lock;

        bool alive() const { return prev->next == this && next->prev == this; }
        bool larger (Avl *x) const { return  node_base > static_cast<Mdb *>(x)->node_base; }
        bool equal  (Avl *x) const { return (node_base ^ static_cast<Mdb *>(x)->node_base) >> max (node_order, static_cast<Mdb *>(x)->node_order) == 0; }

    public:
        Spinlock    node_lock;
        Mdb *       prev;
        Mdb *       next;
        unsigned    dpth;
        Pd *  const node_pd;
        mword const node_base;
        mword const node_order;
        mword       node_attr;
        mword const node_type;
        mword const node_sub;

        explicit Mdb (Pd *p, mword b, mword o = 0, mword a = 0, mword t = 0, mword s = 0) : prev (this), next (this), node_pd (p), node_base (b), node_order (o), node_attr (a), node_type (t), node_sub (s) {}

        static Mdb *lookup (Avl *tree, mword base)
        {
            Mdb *n = 0;
            bool d;

            for (Mdb *m = static_cast<Mdb *>(tree); m; m = static_cast<Mdb *>(m->link (d))) {

                if ((m->node_base ^ base) >> m->node_order == 0)
                    return m;

                if ((d = base > m->node_base) == 0)
                    n = m;
            }

            return n;
        }

        bool insert_node (Mdb *, mword);
        void demote_node (mword);
        bool remove_node();

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
