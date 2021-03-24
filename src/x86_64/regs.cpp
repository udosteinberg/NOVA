/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "hip.hpp"
#include "regs.hpp"
#include "vpid.hpp"

template <> void Cpu_regs::tlb_invalidate<Vmcb>() const
{
    if (vmcb->asid)
        vmcb->tlb_control = 3;
}

template <> void Cpu_regs::tlb_invalidate<Vmcs>() const
{
    auto vpid = Vmcs::vpid();

    if (vpid)
        Vpid::invalidate (Vmcs::ept_vpid.invvpid_sgl ? Invvpid::Type::SGL : Invvpid::Type::ALL, vpid);
}

template <> void Cpu_regs::set_cpu_ctrl0<Vmcb> (uint32 val) const
{
    unsigned const msk = !!cr0_msk<Vmcb>() << 0 | !!cr4_msk<Vmcb>() << 4;

    vmcb->npt_control  = 1;
    vmcb->intercept_cr = (msk << 16) | msk;

    vmcb->intercept_cpu[0] = val | Vmcb::force_ctrl0;
}

template <> void Cpu_regs::set_cpu_ctrl1<Vmcb> (uint32 val) const
{
    vmcb->intercept_cpu[1] = val | Vmcb::force_ctrl1;
}

template <> void Cpu_regs::set_cpu_ctrl0<Vmcs> (uint32 val) const
{
    val |= pio_bitmap * Vmcs::CPU_IO_BITMAP;
    val |= msr_bitmap * Vmcs::CPU_MSR_BITMAP;

    Vmcs::write (Vmcs::Encoding::CPU_CONTROLS0, (val | Vmcs::ctrl_cpu[0].set) & Vmcs::ctrl_cpu[0].clr);
}

template <> void Cpu_regs::set_cpu_ctrl1<Vmcs> (uint32 val) const
{
    Vmcs::write (Vmcs::Encoding::CPU_CONTROLS1, (val | Vmcs::ctrl_cpu[1].set) & Vmcs::ctrl_cpu[1].clr);
}

void Cpu_regs::fpu_ctrl (bool on)
{
    if (Hip::feature (Hip_arch::Feature::VMX)) {

        vmcs->make_current();

        mword cr0 = get_cr0<Vmcs>();
        fpu_on = on;
        set_cr0<Vmcs> (cr0);

        set_exc<Vmcs> (exc_bitmap);

        Vmcs::write (Vmcs::Encoding::CR0_MASK, cr0_msk<Vmcs>());

    } else {

        mword cr0 = get_cr0<Vmcb>();
        fpu_on = on;
        set_cr0<Vmcb> (cr0);

        set_exc<Vmcb> (exc_bitmap);

        set_cpu_ctrl0<Vmcb> (vmcb->intercept_cpu[0]);
    }
}
