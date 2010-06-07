/*
 * Execution Context
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
Slab_cache Ec::cache (sizeof (Ec), 8);

Ec *Ec::current, *Ec::fpowner;

// Constructors
Ec::Ec (Pd *own, mword sel, Pd *p, mword c, mword e, void (*f)()) : Kobject (own, sel, EC), cont (f), utcb (0), pd (p), cpu (c), evt (e)
{
    trace (TRACE_SYSCALL, "EC:%p created (PD:%p Kernel)", this, p);
}

Ec::Ec (Pd *own, mword sel, Pd *p, mword c, mword u, mword s, mword e, bool w) : Kobject (own, sel, EC), pd (p), sc (w ? reinterpret_cast<Sc *>(~0ul) : 0), cpu (c), evt (e)
{
    // Make sure we have a PTAB for this CPU in the PD
    pd->Space_mem::init (c);

    if (u) {

        if (w) {
            cont = static_cast<void (*)()>(0);
            regs.ecx = s;
        } else {
            cont = send_msg<ret_user_iret>;
            regs.cs  = SEL_USER_CODE;
            regs.ds  = SEL_USER_DATA;
            regs.es  = SEL_USER_DATA;
            regs.ss  = SEL_USER_DATA;
            regs.efl = Cpu::EFL_IF;
            regs.esp = s;
        }

        utcb = new Utcb;

        pd->Space_mem::insert (u, 0,
                               Ptab::Attribute (Ptab::ATTR_USER |
                                                Ptab::ATTR_WRITABLE |
                                                Ptab::ATTR_PRESENT),
                               Buddy::ptr_to_phys (utcb));

        regs.dst_portal = NUM_EXC - 2;

        trace (TRACE_SYSCALL, "EC:%p created (PD:%p CPU:%#lx UTCB:%#lx ESP:%lx EVT:%#lx W:%u)", this, p, c, u, s, e, w);

    } else {

        regs.dst_portal = NUM_VMI - 2;

        if (Cpu::feature (Cpu::FEAT_VMX)) {
            regs.vmcs = new Vmcs (reinterpret_cast<mword>(static_cast<Sys_regs *>(&regs) + 1),
                                  Buddy::ptr_to_phys (pd->bmp),
                                  Buddy::ptr_to_phys (pd->cpu_ptab (c)),
                                  Buddy::ptr_to_phys (pd->ept));

            regs.vtlb = new Vtlb;
            regs.ept_ctrl (false);
            cont = send_msg<ret_user_vmresume>;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCS:%p VTLB:%p EPT:%p)", this, p, regs.vmcs, regs.vtlb, pd->ept);
        }

        if (Cpu::feature (Cpu::FEAT_SVM)) {
            regs.vmcb = new Vmcb (Buddy::ptr_to_phys (pd->bmp),
                                  Buddy::ptr_to_phys (pd->mst));
            cont = send_msg<ret_user_vmrun>;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCB:%p VTLB:%p)", this, p, regs.vmcb, regs.vtlb);
        }
    }
}

void Ec::handle_hazard (void (*func)())
{
    if (Cpu::hazard & Cpu::HZD_SCHED) {
        current->cont = func;
        Sc::schedule();
    }

    if (Cpu::hazard & Cpu::HZD_DS_ES) {
        Cpu::hazard &= ~Cpu::HZD_DS_ES;
        asm volatile ("mov %0, %%ds; mov %0, %%es" : : "r" (SEL_USER_DATA));
    }

    if (current->hazard & Cpu::HZD_RECALL) {

        current->hazard &= ~Cpu::HZD_RECALL;

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
}

void Ec::ret_user_sysexit()
{
    if (EXPECT_FALSE ((Cpu::hazard | current->hazard) & (Cpu::HZD_RECALL | Cpu::HZD_SCHED | Cpu::HZD_DS_ES)))
        handle_hazard (ret_user_sysexit);

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
    if (EXPECT_FALSE ((Cpu::hazard | current->hazard) & (Cpu::HZD_RECALL | Cpu::HZD_SCHED)))
        handle_hazard (ret_user_iret);

    Rcu::quiet();

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
    if (EXPECT_FALSE ((Cpu::hazard | current->hazard) & (Cpu::HZD_RECALL | Cpu::HZD_SCHED)))
        handle_hazard (ret_user_vmresume);

    Rcu::quiet();

    current->regs.vmcs->make_current();

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
    if (EXPECT_FALSE ((Cpu::hazard | current->hazard) & (Cpu::HZD_RECALL | Cpu::HZD_SCHED)))
        handle_hazard (ret_user_vmrun);

    Rcu::quiet();

    current->regs.eax = Buddy::ptr_to_phys (current->regs.vmcb);

    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "vmload;"
                  "sti;"
                  "vmrun;"
                  "vmsave;"
                  "jmp entry_svm;"
                  : : "m" (current->regs) : "memory");

    UNREACHED;
}

void Ec::idle()
{
    Rcu::quiet();

    uint64 t1 = rdtsc();
    asm volatile ("sti; hlt; cli" : : : "memory");
    uint64 t2 = rdtsc();

    Counter::cycles_idle += t2 - t1;

    Sc::schedule();
}

void Ec::root_invoke()
{
    // Map root task image
    mword addr = reinterpret_cast<mword>(Ptab::current()->remap (Hip::root_addr));

    Elf_eh *eh = reinterpret_cast<Elf_eh *>(addr);
    if (!Hip::root_addr || *eh->ident != Elf_eh::EH_MAGIC)
        die ("No ELF");

    current->regs.eip = eh->entry;
    current->regs.esp = LINK_ADDR - PAGE_SIZE;

    Elf_ph *ph = reinterpret_cast<Elf_ph *>(addr + eh->ph_offset);

    for (unsigned i = 0; i < eh->ph_count; i++, ph++) {

        if (ph->type != Elf_ph::PT_LOAD)
            continue;

        unsigned attr = !!(ph->flags & Elf_ph::PF_R) << 0 |
                        !!(ph->flags & Elf_ph::PF_W) << 1 |
                        !!(ph->flags & Elf_ph::PF_X) << 2;

        if (ph->f_size != ph->m_size || ph->v_addr % PAGE_SIZE != ph->f_offs % PAGE_SIZE)
            die ("Bad ELF");

        mword p = align_dn (ph->f_offs + Hip::root_addr, PAGE_SIZE);
        mword v = align_dn (ph->v_addr, PAGE_SIZE);
        mword s = align_up (ph->f_size, PAGE_SIZE);

        for (unsigned o; s; s -= 1UL << o, p += 1UL << o, v += 1UL << o)
            Pd::current->delegate<Space_mem>(&Pd::kern, p, v, o = min (max_order (p, s), max_order (v, s)), attr);
    }

    // Map hypervisor information page
    Pd::current->delegate<Space_mem>
                (&Pd::kern,
                 reinterpret_cast<Paddr>(&FRAME_H),
                 LINK_ADDR - PAGE_SIZE,
                 PAGE_BITS, 1);

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
        trace (0, "Killed EC:%p V:%#lx CS:%#lx EIP:%#lx CR2:%#lx ERR:%#lx (%s)",
               current, r->vec, r->cs, r->eip, r->cr2, r->err, reason);
    else
        trace (0, "Killed EC:%p V:%#lx CR0:%#lx CR3:%#lx CR4:%#lx (%s)",
               current, r->vec, r->cr0_shadow, r->cr3_shadow, r->cr4_shadow, reason);

    Sc::schedule (true);
}
