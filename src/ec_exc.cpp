/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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
#include "stdio.h"

void Ec::handle_exc_nm()
{
    trace (TRACE_FPU, "Switch FPU EC:%p (%c) -> EC:%p (%c)",
           fpowner, fpowner && fpowner->utcb ? 'T' : 'V',
           current,            current->utcb ? 'T' : 'V');

    Fpu::enable();

    if (current == fpowner)
        return;

    if (fpowner) {

        // For a vCPU, enable CR0.TS and #NM intercepts
        if (fpowner->utcb == nullptr)
            fpowner->regs.fpu_ctrl (false);

        fpowner->fpu->save();
    }

    // For a vCPU, disable CR0.TS and #NM intercepts
    if (current->utcb == nullptr)
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
    r->REG(fl) &= ~Cpu::EFL_NT;

    return true;
}

bool Ec::handle_exc_gp (Exc_regs *)
{
    if (Cpu::hazard & HZD_TR) {
        Cpu::hazard &= ~HZD_TR;
        Gdt::unbusy_tss();
        asm volatile ("ltr %w0" : : "r" (SEL_TSS_RUN));
        return true;
    }

    return false;
}

bool Ec::handle_exc_pf (Exc_regs *r)
{
    mword addr = r->cr2;

    if (r->err & Hpt::ERR_U)
        return addr < USER_ADDR && Pd::current->Space_mem::loc[Cpu::id].sync_from (Pd::current->Space_mem::hpt, addr, USER_ADDR);

    if (addr < USER_ADDR) {

        if (Pd::current->Space_mem::loc[Cpu::id].sync_from (Pd::current->Space_mem::hpt, addr, USER_ADDR))
            return true;

        if (fixup (r->REG(ip))) {
            r->REG(ax) = addr;
            return true;
        }
    }

    if (addr >= LINK_ADDR && addr < CPU_LOCAL && Pd::current->Space_mem::loc[Cpu::id].sync_from (Hptp (reinterpret_cast<mword>(&PDBR)), addr, CPU_LOCAL))
        return true;

    // Kernel fault in I/O space
    if (addr >= SPC_LOCAL_IOP && addr <= SPC_LOCAL_IOP_E) {
        Space_pio::page_fault (addr, r->err);
        return true;
    }

    // Kernel fault in OBJ space
    if (addr >= SPC_LOCAL_OBJ) {
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
    }

    if (r->user())
        send_msg<ret_user_iret>();

    die ("EXC", r);
}
