/*
 * Protection Domain
 *
 * Copyright (C) 2007-2010, Udo Steinberg <udo@hypervisor.org>
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

#include "extern.h"
#include "gsi.h"
#include "lock_guard.h"
#include "mtrr.h"
#include "pd.h"
#include "pt.h"
#include "sm.h"
#include "stdio.h"
#include "util.h"
#include "vma.h"

// PD Cache
Slab_cache Pd::cache (sizeof (Pd), 8);

// Current PD
Pd *Pd::current;

// Kernel PD
Pd Pd::kern;

// Root PD
Pd *Pd::root;

Pd::Pd() : Kobject (PD, 0), Space_io (0)
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

    // GSI semaphores
    for (unsigned i = 0; i < NUM_GSI; i++)
        Space_obj::insert_root (i);
}

Pd::Pd (unsigned flags) : Kobject (PD, 1), Space_mem (flags), Space_io (flags)
{
    trace (TRACE_SYSCALL, "PD:%p created (F=%#x)", this, flags);
}

void Pd::delegate_mem (Pd *snd, mword const snd_base, mword const rcv_base, mword const ord, mword const attr)
{
    Vma *head = &snd->Space_mem::vma_head;

    for (Vma *vma = head->list_next; vma != head; vma = vma->list_next) {

        mword base = snd_base;

        unsigned order = clamp (vma->base, base, vma->order, ord);

        // If snd_base/ord intersects with VMA then clamp to base/order
        if (order != ~0UL) {

            trace (TRACE_MAP, "Using VMA B:%#lx O:%lu", vma->base, vma->order);

            // Lock the VMA
            Lock_guard <Spinlock> guard (vma->lock);

            // Look up sender mapping to determine physical address
            Paddr phys;
            if (snd == &kern)
                phys = base;
            else {
                size_t size = snd->Space_mem::lookup (base, phys);
                assert (size);
            }

            // Adjust rcv_base by the clamping offset
            Vma *v = new Vma (this, base - snd_base + rcv_base, order, vma->type, attr & vma->attr);
            if (!Space_mem::insert (v, phys))
                delete v;

        } else if (vma->base > snd_base)
            break;
    }
}

void Pd::delegate_obj (Pd *snd, mword const snd_base, mword const rcv_base, mword const ord)
{
    Vma *head = &snd->Space_obj::vma_head;

    for (Vma *vma = head->list_next; vma != head; vma = vma->list_next) {

        mword base = snd_base;

        unsigned order = clamp (vma->base, base, vma->order, ord);

        // If snd_base/ord intersects with VMA then clamp to base/order
        if (order != ~0UL) {

            trace (TRACE_MAP, "Using VMA B:%#lx O:%lu", vma->base, vma->order);

            // Lock the VMA
            Lock_guard <Spinlock> guard (vma->lock);

            // Look up sender mapping to determine Capability
            Capability cap;
            if (snd == &kern) {
                assert (base < NUM_GSI);
                cap = Capability (Gsi::gsi_table[base].sm);
            } else {
                size_t size = snd->Space_obj::lookup (base, cap);
                assert (size);
            }

            Vma *v = new Vma (this, base - snd_base + rcv_base, order);
            if (!Space_obj::insert (v, cap))
                delete v;

        } else if (vma->base > snd_base)
            break;
    }
}

void Pd::delegate_io (Pd *snd, mword const snd_base, mword const ord)
{
    Vma *head = &snd->Space_io::vma_head;

    for (Vma *vma = head->list_next; vma != head; vma = vma->list_next) {

        mword base = snd_base;

        unsigned order = clamp (vma->base, base, vma->order, ord);

        // If snd_base/ord intersects with VMA then clamp to base/order
        if (order != ~0UL) {

            trace (TRACE_MAP, "Using VMA B:%#lx O:%lu", vma->base, vma->order);

            // Lock the VMA
            Lock_guard <Spinlock> guard (vma->lock);

            Vma *v = new Vma (this, base, order);
            if (!Space_io::insert (v))
                delete v;

        } else if (vma->base > snd_base)
            break;
    }
}

void Pd::revoke_mem (mword /*base*/, size_t /*size*/, bool /*self*/)
{
#if 0
    for (size_t i = 0; i < size; i += PAGE_SIZE) {

        Paddr phys;
        if (!current->Space_mem::lookup (base + i, phys))
            continue;

        Mr *mr = Space_mem::array + (phys >> PAGE_BITS);
        Lock_guard <Spinlock> guard (mr->lock);

        Mn *node = mr->node->lookup (current, base + i);
        if (!node)
            continue;

        for (Mn *chld; (chld = node->remove_child()); delete chld)
            chld->pd->Space_mem::remove (chld->cd, PAGE_SIZE);

        if (!self || !current->Space_mem::remove (base + i, PAGE_SIZE) || !node->root())
            continue;

        delete mr->node; mr->node = 0;
    }
#endif
}

void Pd::revoke_io (mword /*base*/, size_t /*size*/, bool /*self*/)
{
#if 0
    for (size_t i = 0; i < size; i++) {

        if (!Space_io::lookup (base + i))
            continue;

        Mr *mr = Space_io::array + base + i;
        Lock_guard <Spinlock> guard (mr->lock);

        Mn *node = mr->node->lookup (current, base + i);
        if (!node)
            continue;

        for (Mn *chld; (chld = node->remove_child()); delete chld)
            chld->pd->Space_io::remove (chld->cd);

        if (!self || !current->Space_io::remove (base + i) || !node->root())
            continue;

        delete mr->node; mr->node = 0;
    }
#endif
}

void Pd::revoke_obj (mword /*base*/, size_t /*size*/, bool /*self*/)
{
#if 0
    for (size_t i = 0; i < size; i++) {

        Capability cap = Space_obj::lookup (base + i);

        switch (cap.type()) {

            case Capability::PT:
                revoke_pt (cap, base + i, self);
                break;

            /*
             * After removing the root capability of a kernel object
             * new references cannot be taken. Wait one RCU epoch and
             * then decrement refcount. If 0, deallocate.
             */

            case Capability::WQ:
                if (current->Space_obj::remove (base + i, cap)) {
                    // XXX: after RCU epoch
                    Wq *wq = cap.obj<Wq>();
                    if (wq->del_ref())
                        delete wq;
                }
                break;

            case Capability::EC:
                if (current->Space_obj::remove (base + i, cap)) {
                    // XXX: after RCU epoch
                    Ec *ec = cap.obj<Ec>();
                    if (ec->del_ref())
                        delete ec;
                }
                break;

            case Capability::SC:
                if (current->Space_obj::remove (base + i, cap))
                    delete cap.obj<Sc>();   // XXX: after RCU epoch
                break;

            case Capability::PD:
                if (current->Space_obj::remove (base + i, cap)) {
                    // XXX: after RCU epoch
                    Pd *pd = cap.obj<Pd>();
                    if (pd->del_ref())
                        delete pd;
                }
                break;

            default:
                break;
        }
    }
#endif
}

void Pd::revoke_pt (Capability /*cap*/, mword /*cd*/, bool /*self*/)
{
#if 0
    Pt *pt = cap.obj<Pt>();
    Lock_guard <Spinlock> guard (pt->lock);

    Mn *node = pt->node.lookup (current, cd);
    if (!node)
        return;

    for (Mn *chld; (chld = node->remove_child()); delete chld)
        chld->pd->Space_obj::remove (chld->cd, cap);

    if (!self || !current->Space_obj::remove (cd, cap) || !node->root())
        return;

    delete pt;          // XXX: after RCU epoch
#endif
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

void Pd::delegate (Pd *pd, Crd rcv, Crd &snd, mword hot)
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
            delegate_mem (pd, sb << PAGE_BITS, rb << PAGE_BITS, o + PAGE_BITS, a);
            break;

        case Crd::IO:
            o = clamp (sb, rb, so, ro);     // XXX: o can be ~0UL
            trace (TRACE_MAP, "MAP I/O PD:%p->%p SB:%#010lx O:%lu", current, this, sb, o);
            delegate_io (pd, rb, o);
            break;

        case Crd::OBJ:
            o = clamp (sb, rb, so, ro, hot);
            trace (TRACE_MAP, "MAP OBJ PD:%p->%p SB:%#010lx RB:%#010lx O:%lu", current, this, sb, rb, o);
            delegate_obj (pd, sb, rb, o);
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
    if (this == root && pd == root)
        pd = &kern;

    while (ti--) {

        mword hot = *src++;
        Crd snd = Crd (*src++);

        delegate (pd, rcv, snd, hot);

        if (dst)
            *dst++ = snd.raw();
    }
}
