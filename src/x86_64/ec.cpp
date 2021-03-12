/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "bits.hpp"
#include "ec.hpp"
#include "elf.hpp"
#include "entry.hpp"
#include "hip.hpp"
#include "multiboot.hpp"
#include "rcu.hpp"
#include "stdio.hpp"
#include "svm.hpp"
#include "vmx.hpp"
#include "vpid.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ec::cache (sizeof (Ec), 32);

Ec *Ec::current, *Ec::fpowner;

// Constructors
Ec::Ec (Pd *own, void (*f)(), cpu_t c) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_GLOBAL), cont (f), utcb (nullptr), pd (own), cpu (c), glb (true), evt (0), timeout (this)
{
    trace (TRACE_SYSCALL, "EC:%p created (PD:%p Kernel)", this, own);
}

Ec::Ec (Pd *, mword, Pd *p, void (*f)(), cpu_t c, unsigned e, mword u, mword s) : Kobject (Kobject::Type::EC, u ? (f ? Kobject::Subtype::EC_GLOBAL : Kobject::Subtype::EC_LOCAL) : Kobject::Subtype::EC_VCPU_REAL), cont (f), pd (p), cpu (c), glb (!!f), evt (e), timeout (this)
{
    // Make sure we have a PTAB for this CPU in the PD
    pd->Space_mem::init (c);

    if (u) {

        if (glb) {
            regs.cs  = SEL_USER_CODE;
            regs.ss  = SEL_USER_DATA;
            regs.rfl = RFL_IF;
            regs.rsp = s;
        } else
            regs.set_sp (s);

        utcb = new Utcb;

        pd->Space_mem::update (u, Kmem::ptr_to_phys (utcb), 0, Paging::Permissions (Paging::R | Paging::W | Paging::U), Memattr::ram());

        regs.dst_portal = NUM_EXC - 2;

        trace (TRACE_SYSCALL, "EC:%p created (PD:%p CPU:%#x UTCB:%#lx ESP:%lx EVT:%#x)", this, p, c, u, s, e);

    } else {

        regs.dst_portal = NUM_VMI - 2;

        if (Hip::hip->feature() & Hip::FEAT_VMX) {

            regs.vmcs = new Vmcs (reinterpret_cast<mword>(sys_regs() + 1),
                                  pd->Space_pio::walk(),
                                  Kmem::ptr_to_phys (pd->loc[c].root_init (false)),
                                  Kmem::ptr_to_phys (pd->ept.root_init (false)),
                                  Vpid::alloc (cpu));

            regs.nst_ctrl<Vmcs>();
            regs.vmcs->clear();
            cont = send_msg<ret_user_vmresume>;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCS:%p)", this, p, regs.vmcs);

        } else if (Hip::hip->feature() & Hip::FEAT_SVM) {

            regs.rax = Kmem::ptr_to_phys (regs.vmcb = new Vmcb (pd->Space_pio::walk(), Kmem::ptr_to_phys (pd->npt.root_init (false))));

            regs.nst_ctrl<Vmcb>();
            cont = send_msg<ret_user_vmrun>;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCB:%p)", this, p, regs.vmcb);
        }
    }
}

void Ec::handle_hazard (mword hzd, void (*func)())
{
    if (hzd & Hazard::RCU)
        Rcu::quiet();

    if (hzd & Hazard::SCHED) {
        current->cont = func;
        Sc::schedule();
    }

    if (hzd & Hazard::RECALL) {
        current->regs.hazard.clr (Hazard::RECALL);

        if (func == ret_user_vmresume) {
            current->regs.dst_portal = NUM_VMI - 1;
            send_msg<ret_user_vmresume>();
        }

        if (func == ret_user_vmrun) {
            current->regs.dst_portal = NUM_VMI - 1;
            send_msg<ret_user_vmrun>();
        }

        if (func == ret_user_sysexit)
            current->redirect_to_iret();

        current->regs.dst_portal = NUM_EXC - 1;
        send_msg<ret_user_iret>();
    }

    if (hzd & Hazard::TSC) {
        current->regs.hazard.clr (Hazard::TSC);

        if (func == ret_user_vmresume) {
            current->regs.vmcs->make_current();
            Vmcs::write (Vmcs::TSC_OFFSET, current->regs.tsc_offset);
        } else
            current->regs.vmcb->tsc_offset = current->regs.tsc_offset;
    }

    if (hzd & Hazard::FPU)
        if (current != fpowner)
            Fpu::disable();
}

void Ec::ret_user_sysexit()
{
    mword hzd = (Cpu::hazard | current->regs.hazard) & (Hazard::RECALL | Hazard::RCU | Hazard::FPU | Hazard::SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_sysexit);

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR) "mov %%r11, %%rsp; mov $0x202, %%r11; sysretq" : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::ret_user_iret()
{
    mword hzd = (Cpu::hazard | current->regs.hazard) & (Hazard::RECALL | Hazard::RCU | Hazard::FPU | Hazard::SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_iret);

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR IRET) : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::ret_user_vmresume()
{
    mword hzd = (Cpu::hazard | current->regs.hazard) & (Hazard::RECALL | Hazard::TSC | Hazard::RCU | Hazard::SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_vmresume);

    current->regs.vmcs->make_current();

    if (EXPECT_FALSE (Pd::current->gtlb.tst (Cpu::id))) {
        Pd::current->gtlb.clr (Cpu::id);
        Pd::current->ept.invalidate();
    }

    if (EXPECT_FALSE (Cr::get_cr2() != current->regs.cr2))
        Cr::set_cr2 (current->regs.cr2);

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR)
                  "vmresume;"
                  "vmlaunch;"
                  "lea %1, %%rsp;"
                  : : "m" (current->regs), "m" (DSTK_TOP) : "memory");

    trace (0, "VM entry failed with error %#x", Vmcs::read<uint32> (Vmcs::VMX_INST_ERROR));

    die ("VMENTRY");
}

void Ec::ret_user_vmrun()
{
    mword hzd = (Cpu::hazard | current->regs.hazard) & (Hazard::RECALL | Hazard::TSC | Hazard::RCU | Hazard::SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_vmrun);

    if (EXPECT_FALSE (Pd::current->gtlb.tst (Cpu::id))) {
        Pd::current->gtlb.clr (Cpu::id);
        current->regs.vmcb->tlb_control = 1;
    }

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR)
                  "clgi;"
                  "sti;"
                  "vmload;"
                  "vmrun;"
                  "vmsave;"
                  EXPAND (SAVE_GPR)
                  "mov %1, %%rax;"
                  "lea %2, %%rsp;"
                  "vmload;"
                  "cli;"
                  "stgi;"
                  "jmp svm_handler;"
                  : : "m" (current->regs), "m" (Vmcb::root), "m" (DSTK_TOP) : "memory");

    UNREACHED;
}

void Ec::idle()
{
    for (;;) {

        mword hzd = Cpu::hazard & (Hazard::RCU | Hazard::SCHED);
        if (EXPECT_FALSE (hzd))
            handle_hazard (hzd, idle);

        Cpu::halt();
    }
}

void Ec::root_invoke()
{
    auto const e { static_cast<Eh const *>(Hptp::map (MMAP_GLB_MAP0, Multiboot::ra)) };
    if (!Multiboot::ra || !e->valid (ELF_MACHINE))
        die ("No ELF");

    current->regs.set_pt (Cpu::id);
    current->regs.set_sp (USER_ADDR - PAGE_SIZE (0));
    current->regs.set_ip (e->entry);

#if 0   // FIXME
    auto c = __atomic_load_n (&e->ph_count, __ATOMIC_RELAXED);
    auto p = static_cast<Ph const *>(Hpt::remap (Hip::root_addr + __atomic_load_n (&e->ph_offset, __ATOMIC_RELAXED)));

    for (unsigned i = 0; i < c; i++, p++) {

        if (p->type == 1) {

            unsigned attr = !!(p->flags & 0x4) << 0 |   // R
                            !!(p->flags & 0x2) << 1 |   // W
                            !!(p->flags & 0x1) << 2;    // X

            if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE (0) != (p->f_offs + Hip::root_addr) % PAGE_SIZE (0))
                die ("Bad ELF");

            mword phys = align_dn (p->f_offs + Hip::root_addr, PAGE_SIZE (0));
            mword virt = align_dn (p->v_addr, PAGE_SIZE (0));
            mword size = align_up (p->v_addr + p->f_size, PAGE_SIZE (0)) - virt;

            for (unsigned long o; size; size -= 1UL << o, phys += 1UL << o, virt += 1UL << o)
                Pd::current->delegate<Space_mem>(&Pd::kern, phys >> PAGE_BITS, virt >> PAGE_BITS, (o = min (max_order (phys, size), max_order (virt, size))) - PAGE_BITS, attr);
        }
    }

    // Map hypervisor information page
    Pd::current->delegate<Space_mem>(&Pd::kern, Kmem::ptr_to_phys (&PAGE_H) >> PAGE_BITS, (USER_ADDR - PAGE_SIZE (0)) >> PAGE_BITS, 0, 1);
#endif

    Space_obj::insert_root (Pd::current);
    Space_obj::insert_root (Ec::current);
    Space_obj::insert_root (Sc::current);

    Console::flush();

    ret_user_sysexit();
}

void Ec::die (char const *reason, Exc_regs *r)
{
    if (current->utcb || current->pd == &Pd::kern)
        trace (0, "Killed EC:%p SC:%p V:%#lx CS:%#lx EIP:%#lx CR2:%#lx ERR:%#lx (%s)",
               current, Sc::current, r->vec, r->cs, r->rip, r->cr2, r->err, reason);
    else
        trace (0, "Killed EC:%p SC:%p V:%#lx CR0:%#lx CR4:%#lx (%s)",
               current, Sc::current, r->vec, r->cr0_shadow, r->cr4_shadow, reason);

    Ec *ec = current->rcap;

    if (ec)
        ec->cont = ec->cont == ret_user_sysexit ? static_cast<void (*)()>(sys_finish<Sys_regs::COM_ABT>) : dead;

    reply (dead);
}
