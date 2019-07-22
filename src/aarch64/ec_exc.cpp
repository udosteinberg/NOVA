/*
 * Execution Context (EC)
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "assert.hpp"
#include "ec_arch.hpp"
#include "event.hpp"
#include "fpu.hpp"
#include "interrupt.hpp"
#include "pd.hpp"
#include "smc.hpp"
#include "stdio.hpp"
#include "vmcb.hpp"

void Ec::fpu_load()
{
    assert (fpu);
    fpu->load();
    set_hazard (HZD_FPU);
}

void Ec::fpu_save()
{
    assert (fpu);
    fpu->save();
    clr_hazard (HZD_FPU);
}

bool Ec::fpu_switch()
{
    assert (!(Cpu::hazard & HZD_FPU));
    assert (fpowner != this);

    if (EXPECT_FALSE (!fpu))
        return false;

    Fpu::enable();

    if (EXPECT_TRUE (fpowner))
        fpowner->fpu_save();

    fpu_load();

    trace (TRACE_FPU, "Switching FPU %p -> %p", static_cast<void *>(fpowner), static_cast<void *>(this));

    fpowner = this;

    return true;
}

void Ec_arch::handle_exc_kern (Exc_regs *r)
{
    auto iss = r->el2.esr & 0x1ffffff;
    auto isv = iss >> 24 & 0x1;
    auto srt = iss >> 16 & 0x1f;
    bool wrt = iss >> 6 & 0x1;

    switch (r->ep()) {

        case 0x21:      // Instruction Abort
            trace (0, "KERN inst abort (ISS:%#llx) at IP:%#llx FAR:%#llx", iss, r->el2.elr, r->el2.far);
            if (isv)
                trace (0, "%s via X%llu (%#lx)", wrt ? "ST" : "LD", srt, r->r[srt]);
            break;

        case 0x25:      // Data Abort
            trace (0, "KERN data abort (ISS:%#llx) at IP:%#llx FAR:%#llx", iss, r->el2.elr, r->el2.far);
            if (isv)
                trace (0, "%s via X%llu (%#lx)", wrt ? "ST" : "LD", srt, r->r[srt]);
            break;

        default:
            trace (0, "KERN exception %#llx (ISS:%#llx) at IP:%#llx", r->ep(), iss, r->el2.elr);
            break;
    }

    current->kill ("exception");
}

void Ec_arch::handle_exc_user (Exc_regs *r)
{
    auto esr = static_cast<uint32>(r->el2.esr);

    bool resolved = false;

    Ec *const self = current;

    // SVC #0 from AArch64 state
    if (EXPECT_TRUE (esr == (VAL_SHIFT (0x15, 26) | BIT (25) | 0)))
        ;

    // SVC #1 from AArch64 state
    else if (esr == (VAL_SHIFT (0x15, 26) | BIT (25) | 1))
        resolved = self->pd == Pd::root && Smc::proxy (r->r);

    // FPU Access
    else if (r->ep() == 0x7)
        resolved = self->fpu_switch();

    if (self->subtype == Kobject::Subtype::EC_VCPU) {
        trace (TRACE_EXCEPTION, "EC:%p VMX %#llx at M:%#x IP:%#llx", static_cast<void *>(self), r->ep(), r->emode(), r->el2.elr);
        self->regs.vmcb->save_gst();
        resolved ? ret_user_vmexit (self) : self->kill ("exception");
    } else {
        trace (TRACE_EXCEPTION, "EC:%p EXC %#llx at M:%#x IP:%#llx", static_cast<void *>(self), r->ep(), r->emode(), r->el2.elr);
        resolved ? ret_user_exception (self) : self->kill ("exception");
    }
}

void Ec_arch::handle_irq_kern()
{
    Interrupt::handler (false);
}

void Ec_arch::handle_irq_user()
{
    Ec *const self = current;

    Event::Selector evt = Interrupt::handler (self->subtype == Kobject::Subtype::EC_VCPU);

    if (self->subtype != Kobject::Subtype::EC_VCPU)
        ret_user_exception (self);

    self->regs.vmcb->save_gst();

    if (evt == Event::Selector::NONE)
        ret_user_vmexit (self);

    self->regs.set_ep (Event::gst_arch + evt);

    self->kill ("irq");
}
