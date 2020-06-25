/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "cpu.hpp"
#include "hip.hpp"
#include "regs.hpp"

void Cpu_regs::svm_set_cpu_pri (uint32_t val) const
{
    unsigned const msk = !!msk_cr0<Vmcb>() << 0 | !!msk_cr4<Vmcb>() << 4;

    vmcb->npt_control  = 1;
    vmcb->intercept_cr = (msk << 16) | msk;

    vmcb->intercept_cpu[0] = val | Vmcb::force_ctrl0;
}

void Cpu_regs::svm_set_cpu_sec (uint32_t val) const
{
    vmcb->intercept_cpu[1] = val | Vmcb::force_ctrl1;
}

void Cpu_regs::vmx_set_cpu_pri (uint32_t val) const
{
    // Force CR8 load/store exiting if not using TPR shadowing
    if (!(val & Vmcs::CPU_TPR_SHADOW))
        val |= Vmcs::CPU_CR8_LOAD | Vmcs::CPU_CR8_STORE;

    Vmcs::write (Vmcs::Encoding::CPU_EXEC_CTRL0, (val | Vmcs::ctrl_cpu[0].set) & Vmcs::ctrl_cpu[0].clr);
}

void Cpu_regs::vmx_set_cpu_sec (uint32_t val) const
{
    Vmcs::write (Vmcs::Encoding::CPU_EXEC_CTRL1, (val | Vmcs::ctrl_cpu[1].set) & Vmcs::ctrl_cpu[1].clr);
}

void Cpu_regs::fpu_ctrl (bool on)
{
    if (Hip::hip->feature() & Hip::FEAT_VMX) {

        vmcs->make_current();

        auto cr0 = vmx_get_gst_cr0();
        exc.fpu_on = on;
        vmx_set_gst_cr0 (cr0);
        vmx_set_msk_cr0();
        vmx_set_bmp_exc();

    } else {

        exc.fpu_on = on;
        svm_set_bmp_exc();
        svm_set_cpu_pri (vmcb->intercept_cpu[0]);
    }
}
