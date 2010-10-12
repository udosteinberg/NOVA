/*
 * Protection Domain
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

#include "initprio.h"
#include "mtrr.h"
#include "pd.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), 8);

Pd *Pd::current;

ALIGNED(8) Pd Pd::kern (&Pd::kern);
ALIGNED(8) Pd Pd::root (&Pd::root, NUM_EXC);

Pd::Pd (Pd *own) : Kobject (own, 0, PD)
{
    hpt.set (reinterpret_cast<mword>(&PDBR) + Hpt::HPT_P);

    // XXX: Do not include HV regions (APIC, IOAPIC, DMAR)

    Mtrr::init();

    Space_mem::insert_root (0, LOAD_ADDR, 7);

    mword base = reinterpret_cast<mword>(&LOAD_E);
    Space_mem::insert_root (base, reinterpret_cast<mword>(&LINK_P) - base, 7);

    base = reinterpret_cast<mword>(&LINK_E);
    Space_mem::insert_root (base, 0 - base, 7);

    // HIP
    Space_mem::insert_root (reinterpret_cast<mword>(&FRAME_H), PAGE_SIZE, 1);

    // I/O Ports
    Space_io::insert_root (0, 16);
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

        mword o, b = base;
        if ((o = clamp (mdb->node_base, b, mdb->node_order, ord)) == ~0UL)
            break;

        Mdb *node = mdb;

        unsigned d = node->dpth;

        for (Mdb *succ;; node = succ) {

            if ((node->node_attr & attr) && (self || node != mdb)) {
                node->node_pd->S::update (node, attr);
                node->demote_node (attr);
            }

            succ = ACCESS_ONCE (node->next);

            if (succ->dpth <= d)
                break;
        }

        Mdb *x = ACCESS_ONCE (node->next);
        assert (x->dpth <= d || (x->dpth == node->dpth + 1 && !(x->node_attr & attr)));

        for (Mdb *succ;; node = succ) {

            if (node->remove_node() && node->node_pd->S::remove_node (node))
                Rcu::submit (node);

            succ = ACCESS_ONCE (node->prev);

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

void Pd::delegate_crd (Pd *pd, Crd rcv, Crd &snd, mword hot)
{
    Crd::Type st = snd.type(), rt = rcv.type();

    if (rt != st) {
        snd = Crd (0);
        return;
    }

    mword sb = snd.base(), rb = rcv.base(), so = snd.order(), ro = rcv.order(), a = snd.attr(), o = 0;

    switch (rt) {

        case Crd::MEM:
            o = clamp (sb, rb, so, ro, hot);
            trace (TRACE_DEL, "DEL MEM PD:%p->%p SB:%#010lx RB:%#010lx O:%#04lx A:%#lx", pd, this, sb, rb, o, a);
            delegate<Space_mem>(pd, sb, rb, o, a, snd.sub());
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

    snd = Crd (rt, rb, o, a);
}

void Pd::revoke_crd (Crd crd, bool self)
{
    Cpu::preempt_enable();

    switch (crd.type()) {

        case Crd::MEM:
            trace (TRACE_REV, "REV MEM PD:%p B:%#010lx O:%#04x A:%#x", this, crd.base(), crd.order(), crd.attr());
            revoke<Space_mem>(crd.base(), crd.order(), crd.attr(), self);
            shootdown();
            break;

        case Crd::IO:
            trace (TRACE_REV, "REV I/O PD:%p B:%#010lx O:%#04x A:%#x", this, crd.base(), crd.order(), crd.attr());
            revoke<Space_io>(crd.base(), crd.order(), crd.attr(), self);
            break;

        case Crd::OBJ:
            trace (TRACE_REV, "REV OBJ PD:%p B:%#010lx O:%#04x A:%#x", this, crd.base(), crd.order(), crd.attr());
            revoke<Space_obj>(crd.base(), crd.order(), crd.attr(), self);
            break;
    }

    Cpu::preempt_disable();
}

void Pd::lookup_crd (Crd &crd)
{
    mword b = crd.base();

    Mdb *mdb = 0;

    switch (crd.type()) {
        case Crd::MEM: mdb = Space_mem::lookup_node (b); break;
        case Crd::IO:  mdb = Space_io::lookup_node (b); break;
        case Crd::OBJ: mdb = Space_obj::lookup_node (b); break;
    }

    crd = mdb ? Crd (crd.type(), mdb->node_base, mdb->node_order, mdb->node_attr) : Crd (0);
}

void Pd::xfer_items (Pd *src, Crd rcv, Xfer *s, Xfer *d, unsigned long ti)
{
    for (Crd crd; ti--; s--) {

        Capability cap; Kobject *obj;

        switch (s->flags() & 1) {

            case 0:     // Translate
                crd = s->type() != Crd::OBJ || s->order() || !src->Space_obj::lookup (s->base(), cap) || (obj = cap.obj())->type() == INVALID || obj->node_pd != this ? Crd (0) : Crd (Crd::OBJ, obj->node_base, obj->node_order, obj->node_attr);
                break;

            case 1:     // Delegate
                delegate_crd (src == &root && s->flags() & 0x800 ? &kern : src, rcv, crd = *s, s->hotspot());
                break;
        };

        if (d)
            *d-- = Xfer (crd, s->flags());
    }
}
