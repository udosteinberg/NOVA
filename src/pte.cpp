/*
 * Page Table Entry (PTE)
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
#include "barrier.hpp"
#include "pte.hpp"
#include "pte_tmpl.hpp"
#include "stdio.hpp"
#include "util.hpp"

template <typename PTT, unsigned L, unsigned B, typename I, typename O>
PTT *Pte<PTT,L,B,I,O>::walk (IAddr v, unsigned n, bool a, bool nc)
{
    auto l = L;

    for (PTT *e = static_cast<decltype (e)>(this), pte;; e = static_cast<decltype (e)>(Kmem::phys_to_ptr (pte.addr())) + (v >> (--l * B + PAGE_BITS) & (BIT (B) - 1))) {

        trace (TRACE_PTE, "%s %#llx: l=%u e=%p val=%#llx", __func__, static_cast<uint64>(v), l, static_cast<void *>(e), static_cast<uint64>(e->val));

        if (l == n)
            return e;

        for (;;) {

            pte.val = __atomic_load_n (&e->val, __ATOMIC_RELAXED);

            assert (!pte.val || pte.large (l) != pte.table (l));

            if (!pte.val && !a)
                return nullptr;

            if (!pte.val || pte.large (l)) {

                PTT *p = !pte.val ? new PTT (0, 0, nc) : new PTT (pte.addr (l) | PTT::page_attr (l - 1, PTT::page_pm (pte.val), PTT::page_ca (pte.val, l), PTT::page_sh (pte.val)), page_size ((l - 1) * B), nc);

                if (EXPECT_FALSE (!p))
                    return nullptr;

                OAddr tmp = Kmem::ptr_to_phys (p) | (l != L) * PTT::ptab_attr();

                if (!__atomic_compare_exchange_n (&e->val, &pte.val, tmp, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                    delete p;
                    continue;
                }

                if (nc)
                    Cache::dcache_clean (e);

                pte.val = tmp;
            }

            break;
        }
    }
}

template <typename PTT, unsigned L, unsigned B, typename I, typename O>
Paging::Permissions Pte<PTT,L,B,I,O>::lookup (IAddr v, OAddr &p, unsigned &o, Memattr::Cacheability &ca, Memattr::Shareability &sh) const
{
    auto l = L; PTT pte;

    for (PTT const *e = static_cast<decltype (e)>(this);; e = static_cast<decltype (e)>(Kmem::phys_to_ptr (pte.addr())) + (v >> (--l * B + PAGE_BITS) & (BIT (B) - 1))) {

        o = l * B;

        pte.val = __atomic_load_n (&e->val, __ATOMIC_RELAXED);

        if (!pte.val)
            return Paging::Permissions (p = 0);

        if (pte.table (l))
            continue;

        p = pte.addr (l) | (v & page_mask (o));

        ca = PTT::page_ca (pte.val, l);
        sh = PTT::page_sh (pte.val);

        return PTT::page_pm (pte.val);
    }
}

template <typename PTT, unsigned L, unsigned B, typename I, typename O>
void Pte<PTT,L,B,I,O>::update (IAddr v, OAddr p, unsigned ord, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh, bool nc)
{
    assert ((v & page_mask (ord)) == 0);
    assert ((p & page_mask (ord)) == 0);

    unsigned const o = min (ord, lim);
    unsigned const l = o / B;
    unsigned const n = BIT (o % B);

    OAddr a = PTT::page_attr (l, pm, ca, sh);

    for (unsigned i = 0; i < BIT (ord - o); i++, v += BIT (o + PAGE_BITS), p += BIT (o + PAGE_BITS)) {

        trace (TRACE_PTE, "%s: l=%u v=%#llx p=%#llx o=%#x a=%#llx", __func__, l, static_cast<uint64>(v), static_cast<uint64>(p), o, static_cast<uint64>(a));

        PTT *e = walk (v, l, a, nc), pte;

        if (!e)
            return;

        OAddr x = a ? p | a : 0;
        OAddr s = a ? page_size (l * B) : 0;

        for (unsigned j = 0; j < n; j++, x += s) {

            pte.val = __atomic_exchange_n (&e[j].val, x, __ATOMIC_SEQ_CST);

            if (!pte.val)
                continue;

            if (pte.table (l))
                static_cast<PTT *>(Kmem::phys_to_ptr (pte.addr()))->deallocate (l - 1);
        }

        if (nc)
            Cache::dcache_clean (e, n * sizeof (*e));
    }

    Barrier::fmb_sync();
}

template <typename PTT, unsigned L, unsigned B, typename I, typename O>
void Pte<PTT,L,B,I,O>::deallocate (unsigned l)
{
    if (!l)
        return;

    for (auto e = static_cast<PTT *>(this); e < static_cast<PTT *>(this) + BIT (B); e++) {

        if (!e->val)
            continue;

        if (e->table (l))
            static_cast<PTT *>(Kmem::phys_to_ptr (e->addr()))->deallocate (l - 1);
    }

    delete this;
}
