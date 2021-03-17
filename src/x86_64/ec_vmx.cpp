/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "ec.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"
#include "vmx.hpp"

void Ec::vmx_exception()
{
    auto const vect_info { Vmcs::read<Vmcs::Encoding::ORG_EVENT_IDENT>() };

    if (vect_info & BIT (31)) {

        Vmcs::write<Vmcs::Encoding::INJ_EVENT_IDENT>(vect_info & ~0x1000);

        if (vect_info & BIT (11))
            Vmcs::write<Vmcs::Encoding::INJ_EVENT_ERROR>(Vmcs::read<Vmcs::Encoding::ORG_EVENT_ERROR>());

        if ((vect_info >> 8 & 0x7) >= 4 && (vect_info >> 8 & 0x7) <= 6)
            Vmcs::write<Vmcs::Encoding::ENT_INST_LEN>(Vmcs::read<Vmcs::Encoding::EXI_INST_LEN>());
    };

    switch (Vmcs::read<Vmcs::Encoding::EXI_EVENT_IDENT>() & BIT_RANGE (10, 0)) {

        case 0x202:         // NMI
            asm volatile ("int $0x2" : : : "memory");
            ret_user_vmresume();

        case 0x307:         // #NM
            handle_exc_nm();
            ret_user_vmresume();
    }

    current->exc_regs().set_ep (Vmcs::VMX_EXC_NMI);

    send_msg<ret_user_vmresume>();
}

void Ec::vmx_extint()
{
    Interrupt::handler (Vmcs::read<Vmcs::Encoding::EXI_EVENT_IDENT>() & BIT_RANGE (7, 0));

    ret_user_vmresume();
}

void Ec::handle_vmx()
{
    current->regs.cr2 = Cr::get_cr2();

    Cpu::hazard = (Cpu::hazard | Hazard::TR) & ~Hazard::FPU;

    auto const reason { Vmcs::read<Vmcs::Encoding::EXI_REASON>() & BIT_RANGE (7, 0) };

    switch (reason) {
        case Vmcs::VMX_EXC_NMI:     vmx_exception();
        case Vmcs::VMX_EXTINT:      vmx_extint();
    }

    current->exc_regs().set_ep (reason);

    send_msg<ret_user_vmresume>();
}

void Ec::failed_vmx()
{
    trace (TRACE_ERROR, "VM entry failed with error %#x", Vmcs::read<Vmcs::Encoding::VMX_INST_ERROR>());

    die ("VM entry failure");
}
