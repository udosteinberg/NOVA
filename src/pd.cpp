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
#include "lock_guard.h"
#include "mdb.h"
#include "mtrr.h"
#include "pd.h"
#include "pt.h"
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

    Space_mem::insert_root (0, 0x100000);
    Space_mem::insert_root (0x100000, LOAD_ADDR - 0x100000);

    mword base = LOAD_ADDR + reinterpret_cast<mword>(&LOAD_SIZE);
    Space_mem::insert_root (base, reinterpret_cast<mword>(&LINK_PHYS) - base);

    base = reinterpret_cast<mword>(&LINK_PHYS) + reinterpret_cast<mword>(&LINK_SIZE);
    Space_mem::insert_root (base, 0 - base);

    // HIP
    Space_mem::insert_root (reinterpret_cast<mword>(&FRAME_H), PAGE_SIZE, 6, 1);

    // I/O Ports
    Space_io::insert_root (0, 16);
}

Pd::Pd (unsigned flags) : Kobject (PD, 1), Space_mem (flags), Space_io (flags)
{
    trace (TRACE_SYSCALL, "PD:%p created (F=%#x)", this, flags);
}

void Pd::delegate_mem (mword const snd_base, mword const rcv_base, mword const ord, mword const attr)
{
    Vma *head = &(privileged() ? &kern : current)->Space_mem::vma_head;

    for (Vma *vma = head->list_next; vma != head; vma = vma->list_next) {

        mword base = snd_base;

        // If snd_base/ord intersects with VMA then clamp to base/order
        if (unsigned order = clamp (base, vma->base, ord, vma->order)) {

            trace (TRACE_MAP, "Using VMA B:%#lx O:%lu", vma->base, vma->order);

            // Lock the VMA
            Lock_guard <Spinlock> guard (vma->lock);

            // Look up sender mapping to determine physical address
            // if we put phys into each VMA node then we do not need to
            // do a PT lookup here.
            Paddr phys;
            if (privileged())
                phys = base;
            else {
                size_t size; bool ok;
                ok = current->Space_mem::lookup (base, size, phys);
                assert (ok);
            }

            // Adjust rcv_base by the clamping offset
            Vma *v = vma->create_child (&(Space_mem::vma_head), this, base - snd_base + rcv_base, order, vma->type, attr & vma->attr);
            if (v)
                Space_mem::insert (v, phys);

        } else if (vma->base > snd_base)
            break;
    }
}

void Pd::delegate_io (mword const snd_base, mword const ord)
{
    Vma *head = &(privileged() ? &kern : current)->Space_io::vma_head;

    for (Vma *vma = head->list_next; vma != head; vma = vma->list_next) {

        mword base = snd_base;

        // If snd_base/ord intersects with VMA then clamp to base/order
        if (unsigned order = clamp (base, vma->base, ord, vma->order)) {

            trace (TRACE_MAP, "Using VMA B:%#lx O:%lu", vma->base, vma->order);

            // Lock the VMA
            Lock_guard <Spinlock> guard (vma->lock);

            Vma *v = vma->create_child (&(Space_io::vma_head), this, base, order, 0, 0);
            if (v)
                Space_io::insert (v);

        } else if (vma->base > snd_base)
            break;
    }
}

void Pd::delegate_obj (mword const snd_base, mword const rcv_base, mword const ord)
{
    size_t size = 1ul << ord;

    for (size_t i = 0; i < size; i++) {

        // Try to find the mapnode corresponding to the send base in the
        // parent address space. RCU protects all pointers until preemption.
        Map_node *parent = current->Space_obj::lookup_node (snd_base + i);
        if (!parent)
            continue;

        Map_node *child = new Map_node (this, rcv_base + i);

        if (!Space_obj::insert (Space_obj::lookup (snd_base + i), parent, child))
            delete child;
        else
            trace (TRACE_MAP, "MAP OBJ B:%#lx -> B:%#lx", parent->base, child->base);
    }
}

#if 0
void Pd::delegate_obj (mword const snd_base, mword const rcv_base, unsigned const ord)
{
    size_t size = 1ul << ord;

    assert (snd_base % size == 0);
    assert (rcv_base % size == 0);

    for (size_t i = 0; i < size; i++) {

        Capability cap = Space_obj::lookup (snd_base + i);
        if (cap.type() != Capability::PT)
            continue;

        Pt *pt = cap.obj<Pt>();
        Lock_guard <Spinlock> guard (pt->lock);

        Mn *node = pt->node.lookup (current, snd_base + i);
        if (!node)
            continue;

        if (Space_obj::insert (rcv_base + i, cap))
            node->create_child (this, rcv_base + i, 0);
    }
}
#endif

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

mword Pd::clamp (mword &snd_base, mword rcv_base, mword snd_ord, mword rcv_ord)
{
    if ((snd_base ^ rcv_base) >> max (snd_ord, rcv_ord))
        return 0;

    snd_base |= rcv_base;

    return min (snd_ord, rcv_ord);
}

mword Pd::clamp (mword &snd_base, mword &rcv_base, mword snd_ord, mword rcv_ord, mword h)
{
    mword s = snd_ord < sizeof (mword) * 8 ? (1ul << snd_ord) - 1 : ~0ul;
    mword r = rcv_ord < sizeof (mword) * 8 ? (1ul << rcv_ord) - 1 : ~0ul;

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

void Pd::delegate (Crd rcv, Crd snd, mword hot)
{
    if (rcv.type() != snd.type()) {
        trace (TRACE_MAP, "CRD mismatch SB:%#010lx O:%u T:%u -> RB:%#010lx O:%u T:%u",
               snd.base(), snd.order(), snd.type(),
               rcv.base(), rcv.order(), rcv.type());
        return;
    }

    mword snd_base = snd.base();
    mword rcv_base = rcv.base();

    mword ord;

    switch (snd.type()) {

        case Crd::MEM:
            snd_base <<= PAGE_BITS;
            rcv_base <<= PAGE_BITS;
            ord = clamp (snd_base, rcv_base, snd.order() + PAGE_BITS, rcv.order() + PAGE_BITS, hot);
            trace (TRACE_MAP, "MAP MEM PD:%p->%p SB:%#010lx RB:%#010lx O:%lu A:%#x", current, this, snd_base, rcv_base, ord, snd.attr());
            delegate_mem (snd_base, rcv_base, ord, snd.attr());
            break;

        case Crd::IO:
            ord = clamp (snd_base, rcv_base, snd.order(), rcv.order());
            trace (TRACE_MAP, "MAP I/O PD:%p->%p SB:%#010lx O:%lu", current, this, snd_base, ord);
            delegate_io (snd_base, ord);
            break;

        case Crd::OBJ:
            ord = clamp (snd_base, rcv_base, snd.order(), rcv.order(), hot);
            trace (TRACE_MAP, "MAP OBJ PD:%p->%p SB:%#010lx RB:%#010lx O:%lu", current, this, snd_base, rcv_base, ord);
            delegate_obj (snd_base, rcv_base, ord);
            break;
    }
}

void Pd::revoke (Crd rev, bool self)
{
    mword base = rev.base();
    mword size = 1ul << rev.order();

    switch (rev.type()) {

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

void Pd::delegate_items (Crd rcv, mword *ptr, unsigned long items)
{
    while (items--) {

        mword hot = *ptr++;
        Crd snd = Crd (*ptr++);

        delegate (rcv, snd, hot);
    }
}
