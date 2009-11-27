/*
 * Execution Context
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "ec.h"
#include "elf.h"
#include "extern.h"
#include "gdt.h"
#include "gsi.h"
#include "hip.h"
#include "memory.h"
#include "ptab.h"
#include "sc.h"
#include "selectors.h"
#include "sm.h"
#include "stdio.h"
#include "svm.h"
#include "utcb.h"
#include "vmx.h"
#include "vtlb.h"

// EC Cache
Slab_cache Ec::cache (sizeof (Ec), 8);

Ec *Ec::current, *Ec::fpowner;

// Constructors
Ec::Ec (Pd *p, void (*c)()) : Kobject (EC, 0), continuation (c), utcb (0), pd (p), wait (0) {}

Ec::Ec (Pd *p, mword c, mword u, mword s, mword e, bool w) : Kobject (EC, 1), pd (p), sc (w ? reinterpret_cast<Sc *>(~0ul) : 0), cpu (c), evt (e), wait (w)
{
    // Make sure we have a PTAB for this CPU in the PD
    pd->Space_mem::init (c);

    if (u) {
        regs.cs  = SEL_USER_CODE;
        regs.ds  = SEL_USER_DATA;
        regs.es  = SEL_USER_DATA;
        regs.ss  = SEL_USER_DATA;
        regs.efl = Cpu::EFL_IF;
        regs.ecx = s;

        utcb = new Utcb;

        pd->Space_mem::insert (u, 0,
                               Ptab::Attribute (Ptab::ATTR_USER |
                                                Ptab::ATTR_WRITABLE),
                               Buddy::ptr_to_phys (utcb));

        regs.dst_portal = NUM_EXC - 2;

        continuation = w ? ret_user_sysexit : send_exc_msg;
        trace (TRACE_SYSCALL, "EC:%p created (PD:%p UTCB:%p)", this, p, utcb);

    } else {

        regs.dst_portal = NUM_VMI - 2;

        if (Cpu::feature (Cpu::FEAT_VMX)) {
            regs.vmcs = new Vmcs (reinterpret_cast<mword>(static_cast<Sys_regs *>(&regs) + 1),
                                  Buddy::ptr_to_phys (pd->cpu_ptab (c)),
                                  Buddy::ptr_to_phys (pd->ept_ptab()));

            regs.vtlb = new Vtlb;
            regs.ept_ctrl (false);
            continuation = send_vmx_msg;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCS:%p VTLB:%p EPT:%p)", this, p, regs.vmcs, regs.vtlb, pd->ept_ptab());
        }

        if (Cpu::feature (Cpu::FEAT_SVM)) {
            regs.vmcb = new Vmcb (Buddy::ptr_to_phys (pd->cpu_ptab (c)));
            continuation = send_svm_msg;
            trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCB:%p VTLB:%p)", this, p, regs.vmcb, regs.vtlb);
        }
    }
}

void Ec::handle_hazard (void (*func)())
{
    if (Cpu::hazard & Cpu::HZD_SCHED) {
        current->continuation = func;
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
            send_vmx_msg();
        }

        if (func == ret_user_vmrun) {
            current->regs.dst_portal = NUM_VMI - 1;
            send_svm_msg();
        }

        // If the EC wanted to leave via SYSEXIT, redirect it to IRET instead.
        if (func == ret_user_sysexit) {
            assert (current->regs.cs == SEL_USER_CODE);
            assert (current->regs.ss == SEL_USER_DATA);
            assert (current->regs.efl & Cpu::EFL_IF);
            current->regs.esp     = current->regs.ecx;
            current->regs.eip     = current->regs.edx;
            current->continuation = ret_user_iret;
        }

        current->regs.dst_portal = NUM_EXC - 1;
        send_exc_msg();
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

    current->regs.vmcs->make_current();

    if (EXPECT_FALSE (Cpu::get_cr2() != current->regs.cr2))
        Cpu::set_cr2 (current->regs.cr2);

    asm volatile ("lea %0, %%esp;"
                  "popa;"
                  "vmresume;"
                  "vmlaunch;"
                  "pusha;"
                  "mov %1, %%esp;"
                  : : "m" (current->regs), "i" (KSTCK_ADDR + PAGE_SIZE) : "memory");

    current->kill (&current->regs, "VMENTRY");
}

void Ec::ret_user_vmrun()
{
    if (EXPECT_FALSE ((Cpu::hazard | current->hazard) & (Cpu::HZD_RECALL | Cpu::HZD_SCHED)))
        handle_hazard (ret_user_vmrun);

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
    uint64 t1 = Cpu::time();

    asm volatile ("sti; hlt; cli" : : : "memory");

    uint64 t2 = Cpu::time();

    Counter::cycles_idle += t2 - t1;

    Sc::schedule();
}

void Ec::root_invoke()
{
    // Delegate GSI portals
    for (unsigned i = 0; i < NUM_GSI; i++)
        Pd::current->Space_obj::insert (Capability (Gsi::gsi_table[i].sm, 0), &Gsi::gsi_table[i].sm->node, new Map_node (Pd::current, NUM_EXC + i));

    // Map hypervisor information page
    Pd::current->delegate_mem (reinterpret_cast<Paddr>(&FRAME_H),
                               LINK_ADDR - PAGE_SIZE,
                               PAGE_BITS, 1);

    // Map root task image
    mword addr = reinterpret_cast<mword>(Ptab::current()->remap (Hip::root_addr));

    Elf_eh *eh = reinterpret_cast<Elf_eh *>(addr);
    if (!Hip::root_addr || *eh->ident != Elf_eh::EH_MAGIC)
        current->kill (&current->regs, "No ELF");

    current->regs.eip = eh->entry;
    current->regs.esp = LINK_ADDR - PAGE_SIZE;

    Elf_ph *ph = reinterpret_cast<Elf_ph *>(addr + eh->ph_offset);

    for (unsigned i = 0; i < eh->ph_count; i++, ph++) {

        if (ph->type != Elf_ph::PT_LOAD)
            continue;

        unsigned attr = !!(ph->flags & Elf_ph::PF_R) << 0 |
                        !!(ph->flags & Elf_ph::PF_W) << 1 |
                        !!(ph->flags & Elf_ph::PF_X) << 2;

        // XXX: Current limitation: fsize == msize so that roottask can be
        //      mapped without having to allocate BSS pages
        assert (ph->f_size == ph->m_size);
        assert (ph->v_addr % PAGE_SIZE == ph->f_offs % PAGE_SIZE);

        for (size_t s = 0; s < ph->f_size; s += PAGE_SIZE)
            Pd::current->delegate_mem (s + align_dn (ph->f_offs + Hip::root_addr, PAGE_SIZE),
                                       s + align_dn (ph->v_addr, PAGE_SIZE),
                                       PAGE_BITS, attr);
    }

    ret_user_iret();
}

void Ec::task_gate_handler()
{
    panic ("Task gate invoked\n");
}

void Ec::exc_handler (Exc_regs *r)
{
    Counter::exc[r->vec]++;

    switch (r->vec) {

        case Cpu::EXC_NM:
            fpu_handler();
            return;

        case Cpu::EXC_TS:
            if (tss_handler (r))
                return;
            break;

        case Cpu::EXC_GP:
            if (Cpu::hazard & Cpu::HZD_TR) {
                Cpu::hazard &= ~Cpu::HZD_TR;
                Gdt::unbusy_tss();
                asm volatile ("ltr %w0" : : "r" (SEL_TSS_RUN));
                return;
            }
            break;

        case Cpu::EXC_PF:
            if (pf_handler (r))
                return;
            break;

        default:
            if (!r->user())
                current->kill (r, "EXC");
    }

    send_exc_msg();
}

void Ec::fpu_handler()
{
    trace (TRACE_FPU, "Switch FPU EC:%p (%c) -> EC:%p (%c)",
           fpowner, fpowner && fpowner->utcb ? 'T' : 'V',
           current,            current->utcb ? 'T' : 'V');

    assert (!Fpu::enabled);

    Fpu::enable();

    if (current == fpowner) {
        assert (fpowner->utcb);     // Should never happen for a vCPU
        return;
    }

    if (fpowner) {

        // For a vCPU, enable CR0.TS and #NM intercepts
        if (fpowner->utcb == 0)
            fpowner->regs.fpu_ctrl (false);

        fpowner->fpu->save();
    }

    // For a vCPU, disable CR0.TS and #NM intercepts
    if (current->utcb == 0)
        current->regs.fpu_ctrl (true);

    if (current->fpu)
        current->fpu->load();
    else {
        current->fpu = new Fpu;
        Fpu::init();
    }

    fpowner = current;
}

bool Ec::tss_handler (Exc_regs *r)
{
    if (r->user())
        return false;

    // Some stupid user did SYSENTER with EFLAGS.NT=1 and IRET faulted
    r->efl &= ~Cpu::EFL_NT;

    return true;
}

bool Ec::pf_handler (Exc_regs *r)
{
    mword addr = r->cr2;

    // Fault caused by user
    if (r->err & Ptab::ERROR_USER) {
        bool s = addr < LINK_ADDR && Pd::current->Space_mem::sync (addr);
        trace (0, "#PF EC:%p EIP:%#010lx ADDR:%#010lx E:%#lx (%s)", current, r->eip, addr, r->err, s ? "fixed" : "ipc");
        return s;
    }

    // Fault caused by kernel

    // #PF in MEM space
    if (addr >= LINK_ADDR && addr < LOCAL_SADDR) {
        Space_mem::page_fault (r->cr2, r->err);
        return true;
    }

    // #PF in I/O space (including delimiter byte)
    if (addr >= IOBMP_SADDR && addr <= IOBMP_EADDR) {
        Space_io::page_fault (r->cr2, r->err);
        return true;
    }

    // #PF in OBJ space
    if (addr >= OBJSP_SADDR) {
        Space_obj::page_fault (r->cr2, r->err);
        return true;
    }

    current->kill (r, "#PF (kernel)");
}

void Ec::kill (Exc_regs *r, char const *reason)
{
    if (utcb || pd == &Pd::kern)
        trace (0, "Killed EC:%p V:%#lx CS:%#lx EIP:%#lx CR2:%#lx ERR:%#lx (%s)",
               this, r->vec, r->cs, r->eip, r->cr2, r->err, reason);
    else
        trace (0, "Killed EC:%p V:%#lx CR0:%#lx CR3:%#lx CR4:%#lx (%s)",
               this, r->vec, r->cr0_shadow, r->cr3_shadow, r->cr4_shadow, reason);

    Sc::schedule (true);
}
