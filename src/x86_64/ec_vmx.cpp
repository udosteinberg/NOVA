/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "counter.hpp"
#include "ec_arch.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"
#include "vmx.hpp"

void Ec_arch::vmx_exception()
{
    uint32 vect_info = Vmcs::read<uint32> (Vmcs::Encoding::ORG_EVENT_IDENT);

    if (vect_info & 0x80000000) {

        Vmcs::write (Vmcs::Encoding::INJ_EVENT_IDENT, vect_info & ~0x1000);

        if (vect_info & 0x800)
            Vmcs::write (Vmcs::Encoding::INJ_EVENT_ERROR, Vmcs::read<uint32> (Vmcs::Encoding::ORG_EVENT_ERROR));

        if ((vect_info >> 8 & 0x7) >= 4 && (vect_info >> 8 & 0x7) <= 6)
            Vmcs::write (Vmcs::Encoding::ENT_INST_LEN, Vmcs::read<uint32> (Vmcs::Encoding::EXI_INST_LEN));
    };

    switch (Vmcs::read<uint32> (Vmcs::Encoding::EXI_EVENT_IDENT) & BIT_RANGE (10, 0)) {

        default:
            exc_regs().set_ep (Vmcs::VMX_EXC_NMI);
            break;

        case 0x202:         // NMI
            asm volatile ("int $0x2" : : : "memory");
            ret_user_vmexit_vmx (this);

        case 0x307:         // #NM
            if (switch_fpu (this))
                ret_user_vmexit_vmx (this);
            break;
    }

    send_msg<ret_user_vmexit_vmx> (this);
}

void Ec_arch::vmx_extint()
{
    Interrupt::handler (Vmcs::read<uint32> (Vmcs::Encoding::EXI_EVENT_IDENT) & BIT_RANGE (7, 0));

    ret_user_vmexit_vmx (this);
}

void Ec_arch::handle_vmx()
{
    Ec *const self { current };

    Cpu::hazard = (Cpu::hazard | Hazard::TR) & ~Hazard::FPU;

    auto reason { Vmcs::read<uint32> (Vmcs::Encoding::EXI_REASON) & BIT_RANGE (7, 0) };

    switch (reason) {
        case Vmcs::VMX_EXC_NMI:     static_cast<Ec_arch *>(self)->vmx_exception();
        case Vmcs::VMX_EXTINT:      static_cast<Ec_arch *>(self)->vmx_extint();
    }

    self->exc_regs().set_ep (reason);

    send_msg<ret_user_vmexit_vmx> (self);
}

void Ec_arch::failed_vmx()
{
    Ec *const self { current };

    trace (TRACE_ERROR, "VM entry failed with error %#x", Vmcs::read<uint32> (Vmcs::Encoding::VMX_INST_ERROR));

    self->kill ("VM entry failure");
}
