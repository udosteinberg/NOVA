/*
 * Execution Context
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

#include "ec.h"
#include "gdt.h"

void Ec::handle_exc_nm()
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

bool Ec::handle_exc_ts (Exc_regs *r)
{
    if (r->user())
        return false;

    // SYSENTER with EFLAGS.NT=1 and IRET faulted
    r->efl &= ~Cpu::EFL_NT;

    return true;
}

bool Ec::handle_exc_gp (Exc_regs *)
{
    if (Cpu::hazard & Cpu::HZD_TR) {
        Cpu::hazard &= ~Cpu::HZD_TR;
        Gdt::unbusy_tss();
        asm volatile ("ltr %w0" : : "r" (SEL_TSS_RUN));
        return true;
    }

    return false;
}

bool Ec::handle_exc_pf (Exc_regs *r)
{
    mword addr = r->cr2;

    // User fault
    if (r->err & Ptab::ERROR_USER)
        return addr < LINK_ADDR && Pd::current->Space_mem::sync (addr);

    // Kernel fault in user region (vTLB)
    if (addr < LINK_ADDR && Pd::current->Space_mem::sync (addr))
        return true;

    // Kernel fault in I/O space
    if (addr >= IOBMP_SADDR && addr <= IOBMP_EADDR) {
        Space_io::page_fault (addr, r->err);
        return true;
    }

    // Kernel fault in OBJ space
    if (addr >= OBJSP_SADDR) {
        Space_obj::page_fault (addr, r->err);
        return true;
    }

    die ("#PF (kernel)", r);
}

void Ec::handle_exc (Exc_regs *r)
{
    Counter::exc[r->vec]++;

    switch (r->vec) {

        case Cpu::EXC_NM:
            handle_exc_nm();
            return;

        case Cpu::EXC_TS:
            if (handle_exc_ts (r))
                return;
            break;

        case Cpu::EXC_GP:
            if (handle_exc_gp (r))
                return;
            break;

        case Cpu::EXC_PF:
            if (handle_exc_pf (r))
                return;
            break;

        default:
            if (!r->user())
                die ("EXC", r);
    }

    send_msg<ret_user_iret>();
}
