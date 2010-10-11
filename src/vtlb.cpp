/*
 * Virtual Translation Lookaside Buffer (VTLB)
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

#include "counter.h"
#include "pd.h"
#include "regs.h"
#include "vpid.h"
#include "vtlb.h"

bool Vtlb::load_pdpte (Exc_regs *regs, uint64 (&v)[4])
{
    mword *pte = reinterpret_cast<mword *>(regs->cr3_shadow);
    mword *val = reinterpret_cast<mword *>(&v);

    for (unsigned i = 0; i < sizeof (v) / sizeof (mword); i++, pte++, val++)
        if (User::peek (pte, *val) != ~0UL)
            return false;

    return true;
}

size_t Vtlb::walk (Exc_regs *regs, mword virt, mword &phys, mword &attr, mword &type)
{
    if (EXPECT_FALSE (!(regs->cr0_shadow & Cpu::CR0_PG))) {
        phys = virt;
        return ~0UL;
    }

    bool pse = regs->cr4_shadow & (Cpu::CR4_PSE | Cpu::CR4_PAE);
    bool pge = regs->cr4_shadow &  Cpu::CR4_PGE;

    type &= ERROR_USER | ERROR_WRITE;

    unsigned lev = levels;

    for (Hpt e, *pte = reinterpret_cast<Hpt *>(regs->cr3_shadow & ~PAGE_MASK);; pte = reinterpret_cast<Hpt *>(e.addr())) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        pte += virt >> shift & ((1UL << bpl) - 1);

        if (User::peek (pte, e) != ~0UL) {
            phys = reinterpret_cast<Paddr>(pte);
            return ~0UL;
        }

        if (EXPECT_FALSE (!e.present()))
            return 0;

        attr &= e.attr();

        if (lev && (!pse || !e.super())) {
            pte->mark (&e, ATTR_ACCESSED);
            continue;
        }

        if (EXPECT_FALSE ((attr & type) != type)) {
            type |= ERROR_PRESENT;
            return 0;
        }

        if (!(type & ERROR_WRITE) && !e.dirty())
            attr &= ~ATTR_WRITABLE;

        pte->mark (&e, (attr & 3) << 5);

        attr |= e.attr() & ATTR_UNCACHEABLE;

        if (EXPECT_TRUE (pge))
            attr |= e.attr() & ATTR_GLOBAL;

        size_t size = 1UL << shift;

        phys = e.addr() | (virt & (size - 1));

        return size;
    }
}

Vtlb::Reason Vtlb::miss (Exc_regs *regs, mword virt, mword &error)
{
    mword phys, attr = ATTR_USER | ATTR_WRITABLE | ATTR_PRESENT;
    Paddr host;

    trace (TRACE_VTLB, "VTLB Miss CR3:%#010lx A:%#010lx E:%#lx", regs->cr3_shadow, virt, error);

    size_t gsize = walk (regs, virt, phys, attr, error);
    if (EXPECT_FALSE (!gsize)) {
        regs->cr2 = virt;
        Counter::vtlb_gpf++;
        return GLA_GPA;
    }

    size_t hsize = Pd::current->ept.lookup (phys, host);
    if (EXPECT_FALSE (!hsize)) {
        regs->ept_fault = phys;
        regs->ept_error = 1UL << !!(error & ERROR_WRITE);
        Counter::vtlb_hpf++;
        return GPA_HPA;
    }

    size_t size = min (gsize, hsize);

    if (gsize > hsize)
        attr |= ATTR_SPLINTER;

    Counter::print (++Counter::vtlb_fill, Console_vga::COLOR_LIGHT_MAGENTA, SPN_VFI);

    unsigned lev = levels;

    for (Vtlb *tlb = regs->vtlb;; tlb = static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()))) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        tlb += virt >> shift & ((1UL << bpl) - 1);

        if (lev) {

            if (size < 1UL << shift) {

                if (tlb->super())
                    tlb->val = Buddy::ptr_to_phys (new Vtlb) | ATTR_GLOBAL | ATTR_PTAB;

                else if (!tlb->present()) {
                    static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()))->flush_ptab (tlb->global());
                    tlb->val = tlb->addr() | ATTR_GLOBAL | ATTR_PTAB;
                }

                tlb->val &= attr | ~ATTR_GLOBAL;
                tlb->val |= attr & ATTR_SPLINTER;

                continue;
            }

            if (!tlb->super())
                delete static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()));

            attr |= ATTR_SUPERPAGE;
        }

        tlb->val = (host & ~((1UL << shift) - 1)) | attr | ATTR_LEAF;

        return SUCCESS;
    }
}

void Vtlb::flush_ptab (unsigned full)
{
    for (Vtlb *tlb = this; tlb < this + (1UL << bpl); tlb++) {

        if (EXPECT_TRUE (!tlb->present()))
            continue;

        if (EXPECT_FALSE (full))
            tlb->val |= ATTR_GLOBAL;

        else if (EXPECT_FALSE (tlb->global()))
            continue;

        tlb->val &= ~ATTR_PRESENT;
    }
}

void Vtlb::flush_addr (mword virt, unsigned long vpid)
{
    unsigned lev = levels;

    for (Vtlb *tlb = this;; tlb = static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()))) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        tlb += virt >> shift & ((1UL << bpl) - 1);

        if (!tlb->present())
            return;

        if (!lev || tlb->splinter()) {
            tlb->val |=  ATTR_GLOBAL;
            tlb->val &= ~ATTR_PRESENT;

            if (vpid)
                Vpid::flush (Vpid::ADDRESS, vpid, virt);

            Counter::print (++Counter::vtlb_flush, Console_vga::COLOR_LIGHT_RED, SPN_VFL);

            return;
        }
    }
}

void Vtlb::flush (unsigned full, unsigned long vpid)
{
    flush_ptab (full);

    if (vpid)
        Vpid::flush (full ? Vpid::CONTEXT_GLOBAL : Vpid::CONTEXT_NOGLOBAL, vpid);

    Counter::print (++Counter::vtlb_flush, Console_vga::COLOR_LIGHT_RED, SPN_VFL);
}
