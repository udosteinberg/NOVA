/*
 * Execution Context (EC)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
    regs.hazard.set (Hazard::FPU);
}

void Ec::fpu_save()
{
    assert (fpu);
    fpu->save();
    regs.hazard.clr (Hazard::FPU);
}

void Ec_arch::handle_exc_kern (Exc_regs *r)
{
    auto const iss { r->el2.esr & BIT_RANGE (24, 0) };

    switch (r->ep()) {

        case 0x21:      // Inst Abort
        case 0x25:      // Data Abort
            trace (TRACE_KILL, "KERN %s abort (ISS:%#lx) at IP:%#lx FAR:%#lx", r->ep() == 0x21 ? "INST" : "DATA", iss, r->el2.elr, r->el2.far);
            break;

        default:
            trace (TRACE_KILL, "KERN exception %#lx (ISS:%#lx) at IP:%#lx", r->ep(), iss, r->el2.elr);
            break;
    }

    current->kill ("exception");
}

void Ec_arch::handle_exc_user (Exc_regs *r)
{
    auto const esr { static_cast<uint32_t>(r->el2.esr) };

    Ec *const self { current };

    bool resolved { false };

    // SVC #0 from AArch64 state
    if (EXPECT_TRUE (esr == (VAL_SHIFT (0x15, 26) | BIT (25) | 0)))
        (*syscall[r->sys.gpr[0] & BIT_RANGE (3, 0)])(self);

    // SVC #1 from AArch64 state
    else if (esr == (VAL_SHIFT (0x15, 26) | BIT (25) | 1))
        resolved = self->get_obj() == Pd::root->get_obj() && Cpu::feature (Cpu::Cpu_feature::EL3) && Smc::proxy (r->sys.gpr);

    // FPU Access
    else if (r->ep() == 0x7)
        resolved = switch_fpu (self);

    trace (TRACE_EXCEPTION, "EC:%p %s %#lx at M:%#x IP:%#lx", static_cast<void *>(self), self->is_vcpu() ? "VMX" : "EXC", r->ep(), r->mode(), r->el2.elr);

    if (self->is_vcpu()) {
        self->regs.vmcb->save_gst();
        resolved ? ret_user_vmexit (self) : send_msg<ret_user_vmexit> (self);
    } else
        resolved ? ret_user_exception (self) : send_msg<ret_user_exception> (self);
}

void Ec_arch::handle_irq_kern()
{
    Interrupt::handler (false);
}

void Ec_arch::handle_irq_user()
{
    Ec *const self { current };

    Event::Selector evt = Interrupt::handler (self->is_vcpu());

    if (!self->is_vcpu())
        ret_user_exception (self);

    self->regs.vmcb->save_gst();

    if (evt == Event::Selector::NONE)
        ret_user_vmexit (self);

    assert (self->regs.vmcb->tmr.cntv_act);

    self->exc_regs().set_ep (Event::gst_arch + evt);

    send_msg<ret_user_vmexit> (self);
}
