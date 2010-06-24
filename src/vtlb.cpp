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
#include "ptab.h"
#include "regs.h"
#include "user.h"
#include "vpid.h"
#include "vtlb.h"

size_t Vtlb::walk (Exc_regs *regs, mword virt, mword error, mword &phys, mword &attr)
{
    if (EXPECT_FALSE (!(regs->cr0_shadow & Cpu::CR0_PG))) {
        phys = virt;
        return ~0ul;
    }

    bool pse = regs->cr4_shadow & (Cpu::CR4_PSE | Cpu::CR4_PAE);
    bool pge = regs->cr4_shadow &  Cpu::CR4_PGE;

    mword bits = (error & (ERROR_USER | ERROR_WRITE)) | ERROR_PRESENT;

    unsigned lev = levels;

    for (Ptab val, *pte = reinterpret_cast<Ptab *>(regs->cr3_shadow);; pte = reinterpret_cast<Ptab *>(val.addr())) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        pte += virt >> shift & ((1ul << bpl) - 1);

        if (peek_user (pte, val) != ~0UL) {         // XXX
            phys = reinterpret_cast<Paddr>(pte);
            return ~0ul;
        }

        attr &= val.attr();

        if (EXPECT_FALSE ((attr & bits) != bits))
            return 0;

        if (EXPECT_FALSE (!val.accessed()))
            Atomic::set_mask<true>(pte->val, ATTR_ACCESSED);

        if (EXPECT_FALSE (lev && (!pse || !val.super())))
            continue;

        if (EXPECT_FALSE (!val.dirty())) {
            if (EXPECT_FALSE (error & ERROR_WRITE))
                Atomic::set_mask<true>(pte->val, ATTR_DIRTY);
            else
                attr &= ~ATTR_WRITABLE;
        }

        attr |= val.attr() & ATTR_UNCACHEABLE;

        if (EXPECT_TRUE (pge))
            attr |= val.attr() & ATTR_GLOBAL;

        size_t size = 1ul << shift;

        phys = val.addr() | (virt & (size - 1));

        return size;
    }
}

Vtlb::Reason Vtlb::miss (Exc_regs *regs, mword virt, mword error)
{
    mword attr = ATTR_USER | ATTR_WRITABLE | ATTR_PRESENT;
    mword phys;
    Paddr host;

    trace (TRACE_VTLB, "VTLB Miss CR3:%#010lx A:%#010lx E:%#lx", regs->cr3_shadow, virt, error);

    size_t gsize = walk (regs, virt, error, phys, attr);
    if (EXPECT_FALSE (!gsize)) {
        regs->cr2 = virt;
        Counter::vtlb_gpf++;
        return GLA_GPA;
    }

    size_t hsize = Pd::current->Space_mem::lookup (phys, host);
    if (EXPECT_FALSE (!hsize)) {
        regs->ept_fault = phys;
        Counter::vtlb_hpf++;
        return GPA_HPA;
    }

    size_t size = min (gsize, hsize);

    if (gsize > hsize)
        attr |= ATTR_SPLINTER;

    Counter::print (++Counter::vtlb_fill, Console_vga::COLOR_LIGHT_MAGENTA, 4);

    unsigned lev = levels;

    for (Vtlb *tlb = regs->vtlb;; tlb = static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()))) {

        unsigned shift = --lev * bpl + PAGE_BITS;
        tlb += virt >> shift & ((1ul << bpl) - 1);

        if (lev) {

            if (size != 1ul << shift) {

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

        tlb->val = (host & ~(size - 1)) | attr | ATTR_LEAF;

        return SUCCESS;
    }
}

void Vtlb::flush_ptab (unsigned full)
{
    for (Vtlb *tlb = this; tlb < this + (1ul << bpl); tlb++) {

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
        tlb += virt >> shift & ((1ul << bpl) - 1);

        if (!tlb->present())
            return;

        if (!lev || tlb->splinter()) {
            tlb->val |=  ATTR_GLOBAL;
            tlb->val &= ~ATTR_PRESENT;

            if (vpid)
                Vpid::flush (Vpid::ADDRESS, vpid, virt);

            Counter::print (++Counter::vtlb_flush, Console_vga::COLOR_LIGHT_RED, 5);

            return;
        }
    }
}

void Vtlb::flush (unsigned full, unsigned long vpid)
{
    flush_ptab (full);

    if (vpid)
        Vpid::flush (full ? Vpid::CONTEXT_GLOBAL : Vpid::CONTEXT_NOGLOBAL, vpid);

    Counter::print (++Counter::vtlb_flush, Console_vga::COLOR_LIGHT_RED, 5);
}
