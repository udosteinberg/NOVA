/*
 * Execution Context
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

#include "bits.h"
#include "ec.h"
#include "elf.h"
#include "hip.h"
#include "initprio.h"
#include "rcu.h"
#include "svm.h"
#include "vmx.h"
#include "vtlb.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ec::cache (sizeof (Ec), 32);

Ec *Ec::current, *Ec::fpowner;

// Constructors
Ec::Ec (Pd *own, mword sel, void (*f)(), unsigned c) : Kobject (EC, own, sel), cont (f), utcb (0), pd (own), cpu (static_cast<uint16>(c)), glb (true), evt (0)
{
    trace (TRACE_SYSCALL, "EC:%p created (PD:%p Kernel)", this, own);
}

Ec::Ec (Pd *own, mword sel, void (*f)(), unsigned c, unsigned e, mword u, mword s) : Kobject (EC, own, sel), cont (f), pd (own), cpu (static_cast<uint16>(c)), glb (!!f), evt (e)
{
    // Make sure we have a PTAB for this CPU in the PD
    pd->Space_mem::init (c);

    if (u) {

        if (glb) {
            regs.cs  = SEL_USER_CODE;
            regs.ds  = SEL_USER_DATA;
            regs.es  = SEL_USER_DATA;
            regs.ss  = SEL_USER_DATA;
            regs.efl = Cpu::EFL_IF;
            regs.esp = s;
        } else
            regs.set_sp (s);

        utcb = new Utcb;

        pd->Space_mem::insert (u, 0, Hpt::HPT_U | Hpt::HPT_W | Hpt::HPT_P, Buddy::ptr_to_phys (utcb));

        regs.dst_portal = NUM_EXC - 2;

        trace (TRACE_SYSCALL, "EC:%p created (PD:%p CPU:%#x UTCB:%#lx ESP:%lx EVT:%#x)", this, own, c, u, s, e);

    } else {

        regs.dst_portal = NUM_VMI - 2;
        regs.vtlb = new Vtlb;

        if (Hip::feature() & Hip::FEAT_VMX) {

            regs.vmcs = new Vmcs (reinterpret_cast<mword>(sys_regs() + 1),
                                  pd->Space_io::walk(),
                                  pd->loc[c].root(),
                                  pd->ept.root());

            regs.nst_ctrl<Vmcs>();
            cont = send_msg<ret_user_vmresume>;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCS:%p VTLB:%p)", this, own, regs.vmcs, regs.vtlb);

        } else if (Hip::feature() & Hip::FEAT_SVM) {

            regs.eax = Buddy::ptr_to_phys (regs.vmcb = new Vmcb (pd->Space_io::walk(), pd->npt.root()));

            regs.nst_ctrl<Vmcb>();
            cont = send_msg<ret_user_vmrun>;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCB:%p VTLB:%p)", this, own, regs.vmcb, regs.vtlb);
        }
    }
}

void Ec::handle_hazard (mword hzd, void (*func)())
{
    if (hzd & HZD_RCU)
        Rcu::quiet();

    if (hzd & HZD_SCHED) {
        current->cont = func;
        Sc::schedule();
    }

    if (hzd & HZD_RECALL) {
        current->regs.clr_hazard (HZD_RECALL);

        if (func == ret_user_vmresume) {
            current->regs.dst_portal = NUM_VMI - 1;
            send_msg<ret_user_vmresume>();
        }

        if (func == ret_user_vmrun) {
            current->regs.dst_portal = NUM_VMI - 1;
            send_msg<ret_user_vmrun>();
        }

        // If the EC wanted to leave via SYSEXIT, redirect it to IRET instead.
        if (func == ret_user_sysexit) {
            assert (current->regs.cs == SEL_USER_CODE);
            assert (current->regs.ss == SEL_USER_DATA);
            assert (current->regs.efl & Cpu::EFL_IF);
            current->regs.esp = current->regs.ecx;
            current->regs.eip = current->regs.edx;
            current->cont     = ret_user_iret;
        }

        current->regs.dst_portal = NUM_EXC - 1;
        send_msg<ret_user_iret>();
    }

    if (hzd & HZD_TSC) {
        current->regs.clr_hazard (HZD_TSC);

        if (func == ret_user_vmresume) {
            current->regs.vmcs->make_current();
            Vmcs::write (Vmcs::TSC_OFFSET,    static_cast<mword>(current->regs.tsc_offset));
            Vmcs::write (Vmcs::TSC_OFFSET_HI, static_cast<mword>(current->regs.tsc_offset >> 32));
        } else
            current->regs.vmcb->tsc_offset = current->regs.tsc_offset;
    }

    if (hzd & HZD_DS_ES) {
        Cpu::hazard &= ~HZD_DS_ES;
        asm volatile ("mov %0, %%ds; mov %0, %%es" : : "r" (SEL_USER_DATA));
    }

    if (hzd & HZD_FPU)
        if (current != fpowner)
            Fpu::disable();
}

void Ec::ret_user_sysexit()
{
    mword hzd = (Cpu::hazard | current->regs.hazard()) & (HZD_RECALL | HZD_RCU | HZD_FPU | HZD_DS_ES | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_sysexit);

    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "sti;"
                  "sysexit"
                  : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::ret_user_iret()
{
    // No need to check HZD_DS_ES because IRET will reload both anyway
    mword hzd = (Cpu::hazard | current->regs.hazard()) & (HZD_RECALL | HZD_RCU | HZD_FPU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_iret);

    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "pop %%gs;"
                  "pop %%fs;"
                  "pop %%es;"
                  "pop %%ds;"
                  "add $8, %%esp;"
                  "iret"
                  : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::ret_user_vmresume()
{
    mword hzd = (Cpu::hazard | current->regs.hazard()) & (HZD_RECALL | HZD_TSC | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_vmresume);

    current->regs.vmcs->make_current();

    if (EXPECT_FALSE (Pd::current->gtlb.chk (Cpu::id))) {
        Pd::current->gtlb.clr (Cpu::id);
        if (current->regs.nst_on)
            Pd::current->ept.flush();
        else
            current->regs.vtlb->flush (true);
    }

    if (EXPECT_FALSE (get_cr2() != current->regs.cr2))
        set_cr2 (current->regs.cr2);

    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "vmresume;"
                  "vmlaunch;"
                  "pusha;"
                  "mov %1, %%esp;"
                  : : "m" (current->regs), "i" (KSTCK_ADDR + PAGE_SIZE) : "memory");

    trace (0, "VM entry failed with error %#lx", Vmcs::read (Vmcs::VMX_INST_ERROR));

    die ("VMENTRY");
}

void Ec::ret_user_vmrun()
{
    mword hzd = (Cpu::hazard | current->regs.hazard()) & (HZD_RECALL | HZD_TSC | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_vmrun);

    if (EXPECT_FALSE (Pd::current->gtlb.chk (Cpu::id))) {
        Pd::current->gtlb.clr (Cpu::id);
        if (current->regs.nst_on)
            current->regs.vmcb->tlb_control = 1;
        else
            current->regs.vtlb->flush (true);
    }

    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "clgi;"
                  "sti;"
                  "vmload;"
                  "vmrun;"
                  "vmsave;"
                  "pusha;"
                  "mov %1, %%eax;"
                  "mov %2, %%esp;"
                  "vmload;"
                  "cli;"
                  "stgi;"
                  "jmp svm_handler;"
                  : : "m" (current->regs), "m" (Vmcb::root), "i" (KSTCK_ADDR + PAGE_SIZE) : "memory");

    UNREACHED;
}

void Ec::idle()
{
    for (;;) {

        mword hzd = Cpu::hazard & (HZD_RCU | HZD_SCHED);
        if (EXPECT_FALSE (hzd))
            handle_hazard (hzd, idle);

        uint64 t1 = rdtsc();
        asm volatile ("sti; hlt; cli" : : : "memory");
        uint64 t2 = rdtsc();

        Counter::cycles_idle += t2 - t1;
    }
}

void Ec::root_invoke()
{
    Eh *e = static_cast<Eh *>(Hpt::remap (Hip::root_addr));
    if (!Hip::root_addr || e->ei_magic != 0x464c457f || e->ei_data != 1 || e->type != 2)
        die ("No ELF");

    unsigned count    = e->ph_count;
    current->regs.eip = e->entry;
    current->regs.esp = USER_ADDR - PAGE_SIZE;

    Ph *p = static_cast<Ph *>(Hpt::remap (Hip::root_addr + e->ph_offset));

    for (unsigned i = 0; i < count; i++, p++) {

        if (p->type == Ph::PT_LOAD) {

            unsigned attr = !!(p->flags & Ph::PF_R) << 0 |
                            !!(p->flags & Ph::PF_W) << 1 |
                            !!(p->flags & Ph::PF_X) << 2;

            if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE != p->f_offs % PAGE_SIZE)
                die ("Bad ELF");

            mword phys = align_dn (p->f_offs + Hip::root_addr, PAGE_SIZE);
            mword virt = align_dn (p->v_addr, PAGE_SIZE);
            mword size = align_up (p->f_size, PAGE_SIZE);

            for (unsigned long o; size; size -= 1UL << o, phys += 1UL << o, virt += 1UL << o)
                Pd::current->delegate<Space_mem>(&Pd::kern, phys >> PAGE_BITS, virt >> PAGE_BITS, (o = min (max_order (phys, size), max_order (virt, size))) - PAGE_BITS, attr);
        }
    }

    // Map hypervisor information page
    Pd::current->delegate<Space_mem>(&Pd::kern, reinterpret_cast<Paddr>(&FRAME_H) >> PAGE_BITS, (USER_ADDR - PAGE_SIZE) >> PAGE_BITS, 0, 1);

    Space_obj::insert_root (Pd::current);
    Space_obj::insert_root (Ec::current);
    Space_obj::insert_root (Sc::current);

    ret_user_iret();
}

void Ec::handle_tss()
{
    panic ("Task gate invoked\n");
}

bool Ec::fixup (mword &eip)
{
    for (mword *ptr = &FIXUP_S; ptr < &FIXUP_E; ptr += 2)
        if (eip == *ptr) {
            eip = *++ptr;
            return true;
        }

    return false;
}

void Ec::die (char const *reason, Exc_regs *r)
{
    if (current->utcb || current->pd == &Pd::kern)
        trace (0, "Killed EC:%p SC:%p V:%#lx CS:%#lx EIP:%#lx CR2:%#lx ERR:%#lx (%s)",
               current, Sc::current, r->vec, r->cs, r->eip, r->cr2, r->err, reason);
    else
        trace (0, "Killed EC:%p SC:%p V:%#lx CR0:%#lx CR3:%#lx CR4:%#lx (%s)",
               current, Sc::current, r->vec, r->cr0_shadow, r->cr3_shadow, r->cr4_shadow, reason);

    Ec *ec = current->rcap;

    if (ec) {
        if (ec->cont == ret_user_sysexit)
            ec->cont = sys_finish<Sys_regs::IPC_ABT>;
        else
            ec->cont = dead;
    }

    reply (dead);
}
