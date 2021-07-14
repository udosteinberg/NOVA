/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "ec.hpp"
#include "interrupt.hpp"
#include "vmx.hpp"

void Ec::vmx_exception()
{
    uint32 vect_info = Vmcs::read<uint32> (Vmcs::IDT_VECT_INFO);

    if (vect_info & 0x80000000) {

        Vmcs::write (Vmcs::ENT_INTR_INFO, vect_info & ~0x1000);

        if (vect_info & 0x800)
            Vmcs::write (Vmcs::ENT_INTR_ERROR, Vmcs::read<uint32> (Vmcs::IDT_VECT_ERROR));

        if ((vect_info >> 8 & 0x7) >= 4 && (vect_info >> 8 & 0x7) <= 6)
            Vmcs::write (Vmcs::ENT_INST_LEN, Vmcs::read<uint32> (Vmcs::EXI_INST_LEN));
    };

    uint32 intr_info = Vmcs::read<uint32> (Vmcs::EXI_INTR_INFO);

    switch (intr_info & 0x7ff) {

        default:
            current->regs.dst_portal = Vmcs::VMX_EXC_NMI;
            break;

        case 0x202:         // NMI
            asm volatile ("int $0x2" : : : "memory");
            ret_user_vmresume();

        case 0x307:         // #NM
            handle_exc_nm();
            ret_user_vmresume();
    }

    send_msg<ret_user_vmresume>();
}

void Ec::vmx_extint()
{
    Interrupt::handler (Vmcs::read<uint32> (Vmcs::EXI_INTR_INFO) & BIT_RANGE (7, 0));

    ret_user_vmresume();
}

void Ec::handle_vmx()
{
    Cpu::hazard = (Cpu::hazard | Hazard::TR) & ~Hazard::FPU;

    uint32 reason = Vmcs::read<uint32> (Vmcs::EXI_REASON) & 0xff;

    switch (reason) {
        case Vmcs::VMX_EXC_NMI:     vmx_exception();
        case Vmcs::VMX_EXTINT:      vmx_extint();
    }

    current->regs.dst_portal = reason;

    send_msg<ret_user_vmresume>();
}
