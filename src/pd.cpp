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

Pd::Pd (Pd *own) : Kobject (own, 0, PD), Space_mem (Ptab::master())
{
    // XXX: Do not include HV regions (APIC, IOAPIC, DMAR)

    Mtrr::init();

    Space_mem::insert_root (0, 0x100000, 7);
    Space_mem::insert_root (0x100000, LOAD_ADDR - 0x100000, 7);

    mword base = LOAD_ADDR + reinterpret_cast<mword>(&LOAD_SIZE);
    Space_mem::insert_root (base, reinterpret_cast<mword>(&LINK_PHYS) - base, 7);

    base = reinterpret_cast<mword>(&LINK_PHYS) + reinterpret_cast<mword>(&LINK_SIZE);
    Space_mem::insert_root (base, 0 - base, 7);

    // HIP
    Space_mem::insert_root (reinterpret_cast<mword>(&FRAME_H), PAGE_SIZE, 1);

    // I/O Ports
    Space_io::insert_root (0, 16);
}

Pd::Pd (Pd *own, mword sel) : Kobject (own, sel, PD), Space_mem (new Ptab)
{
    trace (TRACE_SYSCALL, "PD:%p created", this);
}

template <typename S>
void Pd::delegate (Pd *snd, mword const snd_base, mword const rcv_base, mword const ord, mword const attr, mword const sub)
{
    Mdb *mdb;
    for (mword addr = snd_base; (mdb = snd->S::lookup_node (addr)); addr = mdb->node_base + (1UL << mdb->node_order)) {

        mword o, b = snd_base;
        if ((o = clamp (mdb->node_base, b, mdb->node_order, ord)) == ~0UL)
            break;

        Mdb *node = new Mdb (this, b - snd_base + rcv_base, o, 0, mdb->node_type);

        if (!S::insert_node (node)) {
            delete node;
            continue;
        }

        if (!node->insert_node (mdb, attr)) {
            //S::remove_node (a);
            delete node;
            continue;
        }

        S::update (node, snd->S::lookup_obj (b, snd == &kern), 0, sub);
    }
}

template <typename S>
void Pd::revoke (mword const base, mword const ord, mword const attr, bool self)
{
    Mdb *mdb;
    for (mword addr = base; (mdb = S::lookup_node (addr)); addr = mdb->node_base + (1UL << mdb->node_order)) {

        mword o, b = base;
        if ((o = clamp (mdb->node_base, b, mdb->node_order, ord)) == ~0UL)
            break;

        Mdb *node = mdb;

        unsigned d = node->dpth;

        for (Mdb *succ;; node = succ) {

            if ((node->node_attr & attr) && (self || node != mdb)) {
                node->node_pd->S::update (node, node->node_pd->S::lookup_obj (node->node_base, false), attr, ~0UL);
                node->demote_node (attr);
            }

            succ = ACCESS_ONCE (node->next);

            if (succ->dpth <= d)
                break;
        }

        Mdb *x = ACCESS_ONCE (node->next);
        assert (x->dpth <= d || (x->dpth == node->dpth + 1 && !(x->node_attr & attr)));

        for (Mdb *succ;; node = succ) {

            if (node->remove_node() && !node->node_pd->S::remove_node (node)) {}

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

void Pd::delegate_item (Pd *pd, Crd rcv, Crd &snd, mword hot)
{
    Crd::Type st = snd.type(), rt = rcv.type();

    if (rt != st) {
        snd = Crd (0);
        return;
    }

    mword sb = snd.base(), rb = rcv.base(), so = snd.order(), ro = rcv.order(), a = snd.attr(), o = 0;

    switch (rt) {

        case Crd::MEM:
            o = clamp (sb, rb, so, ro, hot >> PAGE_BITS);
            trace (TRACE_MAP, "MAP MEM PD:%p->%p SB:%#010lx RB:%#010lx O:%lu A:%#lx", current, this, sb << PAGE_BITS, rb << PAGE_BITS, o + PAGE_BITS, a);
            delegate<Space_mem>(pd, sb, rb, o, a, snd.sub());
            break;

        case Crd::IO:
            o = clamp (sb, rb, so, ro);
            trace (TRACE_MAP, "MAP I/O PD:%p->%p SB:%#010lx O:%lu A:%#lx", current, this, sb, o, a);
            delegate<Space_io>(pd, rb, rb, o, a);
            break;

        case Crd::OBJ:
            o = clamp (sb, rb, so, ro, hot);
            trace (TRACE_MAP, "MAP OBJ PD:%p->%p SB:%#010lx RB:%#010lx O:%lu A:%#lx", current, this, sb, rb, o, a);
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
            trace (TRACE_UNMAP, "UNMAP MEM PD:%p B:%#010lx O:%#x A:%#x", this, crd.base() << PAGE_BITS, crd.order() + PAGE_BITS, crd.attr());
            revoke<Space_mem>(crd.base(), crd.order(), crd.attr(), self);
            shootdown();
            break;

        case Crd::IO:
            trace (TRACE_UNMAP, "UNMAP I/O PD:%p B:%#010lx O:%#x A:%#x", this, crd.base(), crd.order(), crd.attr());
            revoke<Space_io>(crd.base(), crd.order(), crd.attr(), self);
            break;

        case Crd::OBJ:
            trace (TRACE_UNMAP, "UNMAP OBJ PD:%p B:%#010lx O:%#x A:%#x", this, crd.base(), crd.order(), crd.attr());
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

    crd = !mdb || clamp (mdb->node_base, b, mdb->node_order, 0) == ~0UL ? Crd (0) : Crd (crd.type(), mdb->node_base, mdb->node_order, mdb->node_attr);
}

void Pd::delegate_items (Pd *pd, Crd rcv, mword *src, mword *dst, unsigned long ti)
{
    if (this == &root && pd == &root)
        pd = &kern;

    while (ti--) {

        mword hot = *src++;
        Crd snd = Crd (*src++);

        delegate_item (pd, rcv, snd, hot);

        if (dst)
            *dst++ = snd.raw();
    }
}
