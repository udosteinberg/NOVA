/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "hazards.hpp"
#include "interrupt.hpp"
#include "smmu.hpp"
#include "vectors.hpp"
#include "vmx.hpp"

void Ec_arch::vmx_exception()
{
    uint32 vect_info = Vmcs::read<uint32> (Vmcs::Encoding::IDT_VECT_INFO);

    if (vect_info & 0x80000000) {

        Vmcs::write (Vmcs::Encoding::ENT_INTR_INFO, vect_info & ~0x1000);

        if (vect_info & 0x800)
            Vmcs::write (Vmcs::Encoding::ENT_INTR_ERROR, Vmcs::read<uint32> (Vmcs::Encoding::IDT_VECT_ERROR));

        if ((vect_info >> 8 & 0x7) >= 4 && (vect_info >> 8 & 0x7) <= 6)
            Vmcs::write (Vmcs::Encoding::ENT_INST_LEN, Vmcs::read<uint32> (Vmcs::Encoding::EXI_INST_LEN));
    };

    uint32 intr_info = Vmcs::read<uint32> (Vmcs::Encoding::EXI_INTR_INFO);

    switch (intr_info & 0x7ff) {

        default:
            current->regs.set_ep (Vmcs::VMX_EXC_NMI);
            break;

        case 0x202:         // NMI
            asm volatile ("int $0x2" : : : "memory");
            ret_user_vmexit_vmx (current);

        case 0x307:         // #NM
            if (current->fpu_switch())
                ret_user_vmexit_vmx (current);
            break;
    }

    send_msg<ret_user_vmexit_vmx> (current);
}

void Ec_arch::vmx_extint()
{
    uint32 vector = Vmcs::read<uint32> (Vmcs::Encoding::EXI_INTR_INFO) & 0xff;

    if (vector >= VEC_IPI)
        Interrupt::handle_ipi (vector);
    else if (vector >= VEC_MSI)
        Smmu::vector (vector);
    else if (vector >= VEC_LVT)
        Interrupt::handle_lvt (vector);
    else if (vector >= VEC_GSI)
        Interrupt::handle_gsi (vector);

    ret_user_vmexit_vmx (current);
}

void Ec_arch::handle_vmx()
{
    // Msr::IA32_KERNEL_GS_BASE can change without VM exit because of SWAPGS
    current->regs.msr.kernel_gs_base = Msr::read (Msr::IA32_KERNEL_GS_BASE);

    Msr::State::make_current (current->regs.msr, Cpu::msr);

    Cpu::hazard = (Cpu::hazard | HZD_DS_ES | HZD_TR) & ~HZD_FPU;

    uint32 reason = Vmcs::read<uint32> (Vmcs::Encoding::EXI_REASON) & 0xff;

    Counter::vmi[reason].inc();

    switch (reason) {
        case Vmcs::VMX_EXC_NMI:     vmx_exception();
        case Vmcs::VMX_EXTINT:      vmx_extint();
    }

    current->regs.set_ep (reason);

    send_msg<ret_user_vmexit_vmx> (current);
}
