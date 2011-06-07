/*
 * Mapping Database
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
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
#include "rcu.h"
#include "slab.h"
#include "spinlock.h"
#include "util.h"

class Pd;

class Mdb : public Avl, public Rcu_elem
{
    private:
        static Slab_cache   cache;
        static Spinlock     lock;

        bool alive() const { return prev->next == this && next->prev == this; }

        static void free (Rcu_elem *e)
        {
            Mdb *m = static_cast<Mdb *>(e);
            delete m;
        }

    public:
        Spinlock    node_lock;
        uint16      dpth;
        Mdb *       prev;
        Mdb *       next;
        Mdb *       prnt;
        Pd *  const node_pd;
        mword const node_phys;
        mword const node_base;
        mword const node_order;
        mword       node_attr;
        mword const node_type;
        mword const node_sub;

        ALWAYS_INLINE
        inline bool larger (Mdb *x) const { return  node_base > x->node_base; }

        ALWAYS_INLINE
        inline bool equal  (Mdb *x) const { return (node_base ^ x->node_base) >> max (node_order, x->node_order) == 0; }

        NOINLINE
        explicit Mdb (Pd *pd, mword p, mword b, mword a, void (*f)(Rcu_elem *)) : Rcu_elem (f), prev (this), next (this), prnt (0), node_pd (pd), node_phys (p), node_base (b), node_order (0), node_attr (a), node_type (0), node_sub (0) {}

        NOINLINE
        explicit Mdb (Pd *pd, mword p, mword b, mword o = 0, mword a = 0, mword t = 0, mword s = 0) : Rcu_elem (free), prev (this), next (this), prnt (0), node_pd (pd), node_phys (p), node_base (b), node_order (o), node_attr (a), node_type (t), node_sub (s) {}

        static Mdb *lookup (Avl *tree, mword base, bool next)
        {
            Mdb *n = 0;
            bool d;

            for (Mdb *m = static_cast<Mdb *>(tree); m; m = static_cast<Mdb *>(m->lnk[d])) {

                if ((m->node_base ^ base) >> m->node_order == 0)
                    return m;

                if ((d = base > m->node_base) == 0 && next)
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
