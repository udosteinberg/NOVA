/*
 * Protection Domain
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

#include "initprio.h"
#include "mtrr.h"
#include "pd.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), 32);

Pd *Pd::current;

ALIGNED(32) Pd Pd::kern (&Pd::kern);
ALIGNED(32) Pd Pd::root (&Pd::root, NUM_EXC, perm);

Pd::Pd (Pd *own) : Kobject (PD, own, 0)
{
    hpt = Hptp (reinterpret_cast<mword>(&PDBR));

    Mtrr::init();

    mword base = reinterpret_cast<mword>(&LINK_E) >> PAGE_BITS;
    Space_mem::insert_root (0, reinterpret_cast<mword>(&LINK_P) >> PAGE_BITS, 7);
    Space_mem::insert_root (base, (1UL << 20) - base, 7);

    // HIP
    Space_mem::insert_root (reinterpret_cast<mword>(&FRAME_H) >> PAGE_BITS, 1, 1);

    // I/O Ports
    Space_io::addreg (0, 1UL << 16, 7);
}

template <typename S>
void Pd::delegate (Pd *snd, mword const snd_base, mword const rcv_base, mword const ord, mword const attr, mword const sub)
{
    Mdb *mdb;
    for (mword addr = snd_base; (mdb = snd->S::lookup_node (addr, true)); addr = mdb->node_base + (1UL << mdb->node_order)) {

        mword o, b = snd_base;
        if ((o = clamp (mdb->node_base, b, mdb->node_order, ord)) == ~0UL)
            break;

        Mdb *node = new Mdb (this, b - mdb->node_base + mdb->node_phys, b - snd_base + rcv_base, o, 0, mdb->node_type, sub);

        if (!S::insert_node (node)) {
            delete node;
            continue;
        }

        if (!node->insert_node (mdb, attr)) {
            //S::remove_node (a);
            delete node;
            continue;
        }

        S::update (node);
    }
}

template <typename S>
void Pd::revoke (mword const base, mword const ord, mword const attr, bool self)
{
    Mdb *mdb;
    for (mword addr = base; (mdb = S::lookup_node (addr, true)); addr = mdb->node_base + (1UL << mdb->node_order)) {

        mword o, p, b = base;
        if ((o = clamp (mdb->node_base, b, mdb->node_order, ord)) == ~0UL)
            break;

        Mdb *node = mdb;

        unsigned d = node->dpth; bool demote = false;

        for (Mdb *ptr;; node = ptr) {

            if (node->dpth == d + !self)
                demote = clamp (node->node_phys, p = b - mdb->node_base + mdb->node_phys, node->node_order, o) != ~0UL;

            if (demote && node->node_attr & attr) {
                node->node_pd->S::update (node, attr);
                node->demote_node (attr);
            }

            ptr = ACCESS_ONCE (node->next);

            if (ptr->dpth <= d)
                break;
        }

        Mdb *x = ACCESS_ONCE (node->next);
        assert (x->dpth <= d || (x->dpth == node->dpth + 1 && !(x->node_attr & attr)));

        for (Mdb *ptr;; node = ptr) {

            if (node->remove_node() && node->node_pd->S::remove_node (node))
                Rcu::call (node);

            ptr = ACCESS_ONCE (node->prev);

            if (node->dpth <= d)
                break;
        }

        assert (node == mdb);
    }
}

mword Pd::clamp (mword snd_base, mword &rcv_base, mword snd_ord, mword rcv_ord)
{
    if ((snd_base ^ rcv_base) >> max (snd_ord, rcv_ord))
        return ~0UL;

    rcv_base |= snd_base;

    return min (snd_ord, rcv_ord);
}

mword Pd::clamp (mword &snd_base, mword &rcv_base, mword snd_ord, mword rcv_ord, mword h)
{
    assert (snd_ord < sizeof (mword) * 8);
    assert (rcv_ord < sizeof (mword) * 8);

    mword s = (1ul << snd_ord) - 1;
    mword r = (1ul << rcv_ord) - 1;

    snd_base &= ~s;
    rcv_base &= ~r;

    if (EXPECT_TRUE (s < r)) {
        rcv_base |= h & r & ~s;
        return snd_ord;
    } else {
        snd_base |= h & s & ~r;
        return rcv_ord;
    }
}

void Pd::xlt_crd (Pd *pd, Crd xlt, Crd &crd)
{
    if (crd.type() == xlt.type()) {

        Mdb *mdb, *node;

        mword sb = crd.base(), so = crd.order(), rb = xlt.base(), ro = xlt.order();

        for (node = mdb = pd->lookup_crd (crd); node; node = node->prnt)
            if (node->node_pd == this && node != mdb)
                if ((ro = clamp (node->node_base, rb, node->node_order, ro)) != ~0UL)
                    break;

        if (node) {

            so = clamp (mdb->node_base, sb, mdb->node_order, so);
            sb = (sb - mdb->node_base) + (mdb->node_phys - node->node_phys) + node->node_base;

            if ((ro = clamp (sb, rb, so, ro)) != ~0UL) {
                crd = Crd (crd.type(), rb, ro, mdb->node_attr);
                return;
            }
        }
    }

    crd = Crd (0);
}

void Pd::del_crd (Pd *pd, Crd del, Crd &crd, mword sub, mword hot)
{
    Crd::Type st = crd.type(), rt = del.type();

    if (rt != st) {
        crd = Crd (0);
        return;
    }

    mword sb = crd.base(), so = crd.order(), rb = del.base(), ro = del.order(), a = crd.attr(), o = 0;

    switch (rt) {

        case Crd::MEM:
            o = clamp (sb, rb, so, ro, hot);
            trace (TRACE_DEL, "DEL MEM PD:%p->%p SB:%#010lx RB:%#010lx O:%#04lx A:%#lx", pd, this, sb, rb, o, a);
            delegate<Space_mem>(pd, sb, rb, o, a, sub);
            break;

        case Crd::IO:
            o = clamp (sb, rb, so, ro);
            trace (TRACE_DEL, "DEL I/O PD:%p->%p SB:%#010lx RB:%#010lx O:%#04lx A:%#lx", pd, this, rb, rb, o, a);
            delegate<Space_io>(pd, rb, rb, o, a);
            break;

        case Crd::OBJ:
            o = clamp (sb, rb, so, ro, hot);
            trace (TRACE_DEL, "DEL OBJ PD:%p->%p SB:%#010lx RB:%#010lx O:%#04lx A:%#lx", pd, this, sb, rb, o, a);
            delegate<Space_obj>(pd, sb, rb, o, a);
            break;
    }

    crd = Crd (rt, rb, o, a);
}

void Pd::rev_crd (Crd crd, bool self)
{
    Cpu::preempt_enable();

    switch (crd.type()) {

        case Crd::MEM:
            trace (TRACE_REV, "REV MEM PD:%p B:%#010lx O:%#04x A:%#04x %s", this, crd.base(), crd.order(), crd.attr(), self ? "+" : "-");
            revoke<Space_mem>(crd.base(), crd.order(), crd.attr(), self);
            shootdown();
            break;

        case Crd::IO:
            trace (TRACE_REV, "REV I/O PD:%p B:%#010lx O:%#04x A:%#04x %s", this, crd.base(), crd.order(), crd.attr(), self ? "+" : "-");
            revoke<Space_io>(crd.base(), crd.order(), crd.attr(), self);
            break;

        case Crd::OBJ:
            trace (TRACE_REV, "REV OBJ PD:%p B:%#010lx O:%#04x A:%#04x %s", this, crd.base(), crd.order(), crd.attr(), self ? "+" : "-");
            revoke<Space_obj>(crd.base(), crd.order(), crd.attr(), self);
            break;
    }

    Cpu::preempt_disable();
}

void Pd::xfer_items (Pd *src, Crd xlt, Crd del, Xfer *s, Xfer *d, unsigned long ti)
{
    for (Crd crd; ti--; s--) {

        crd = *s;

        switch (s->flags() & 1) {

            case 0:
                xlt_crd (src, xlt, crd);
                break;

            case 1:
                del_crd (src == &root && s->flags() & 0x800 ? &kern : src, del, crd, s->flags() >> 9 & 3, s->hotspot());
                break;
        };

        if (d)
            *d-- = Xfer (crd, s->flags());
    }
}
