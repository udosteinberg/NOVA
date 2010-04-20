/*
 * Protection Domain
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

#include "initprio.h"
#include "mtrr.h"
#include "pd.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Pd::cache (sizeof (Pd), 8);

Pd *Pd::current;

Pd Pd::kern (&Pd::kern, 0);
Pd Pd::root (&Pd::root, NUM_EXC, 0x3);

Pd::Pd (Pd *own, mword sel) : Kobject (own, sel, PD), Space_io (0)
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

Pd::Pd (Pd *own, mword sel, unsigned flags) : Kobject (own, sel, PD), Space_mem (flags), Space_io (flags)
{
    trace (TRACE_SYSCALL, "PD:%p created (F=%#x)", this, flags);
}

template <typename S>
void Pd::delegate (Pd *snd, mword const snd_base, mword const rcv_base, mword const ord, mword const attr)
{
    Mdb *mdb;
    for (mword addr = snd_base; (mdb = snd->S::lookup_node (addr)); addr = mdb->node_base + (1UL << mdb->node_order)) {

        mword o, b = snd_base;
        if ((o = clamp (mdb->node_base, b, mdb->node_order, ord)) == ~0UL)
            break;

//        trace (0, "A:%#010lx B:%#010lx O:%lu vs. B:%#010lx O:%lu -> %d", addr, mdb->node_base, mdb->node_order, b, ord, o);

        // Adjust rcv_base by the clamping offset
        Mdb *a = new Mdb (this, b - snd_base + rcv_base, o, mdb->node_attr & attr, mdb->node_type);
        if (!S::insert_node (a)) {
            delete a;
            continue;
        }

        // Look up and insert sender mapping
        S::insert (a, snd->S::lookup_obj (b, snd == &kern));
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
            trace (TRACE_MAP, "MAP MEM PD:%p->%p SB:%#010lx RB:%#010lx O:%lu A:%#lx", current, this, sb, rb, o, a);
            delegate<Space_mem>(pd, sb << PAGE_BITS, rb << PAGE_BITS, o + PAGE_BITS, a);
            break;

        case Crd::IO:
            o = clamp (sb, rb, so, ro);     // XXX: o can be ~0UL
            trace (TRACE_MAP, "MAP I/O PD:%p->%p SB:%#010lx O:%lu", current, this, sb, o);
            delegate<Space_io>(pd, rb, rb, o);
            break;

        case Crd::OBJ:
            o = clamp (sb, rb, so, ro, hot);
            trace (TRACE_MAP, "MAP OBJ PD:%p->%p SB:%#010lx RB:%#010lx O:%lu", current, this, sb, rb, o);
            delegate<Space_obj>(pd, sb, rb, o);
            break;
    }

    snd = Crd (rt, rb, o, a);
}

void Pd::revoke (Crd r, bool self)
{
    mword base = r.base();
    mword size = 1ul << r.order();

    switch (r.type()) {

        case Crd::MEM:
            base <<= PAGE_BITS;
            trace (TRACE_MAP, "UNMAP MEM PD:%p B:%#010lx S:%#lx", current, base, size);
            revoke_mem (base, size, self);
            break;

        case Crd::IO:
            trace (TRACE_MAP, "UNMAP I/O PD:%p B:%#010lx S:%#lx", current, base, size);
            revoke_io (base, size, self);
            break;

        case Crd::OBJ:
            trace (TRACE_MAP, "UNMAP OBJ PD:%p B:%#010lx S:%#lx", current, base, size);
            revoke_obj (base, size, self);
            break;
    }
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
