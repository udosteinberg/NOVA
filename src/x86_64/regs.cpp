/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "cpu.hpp"
#include "hip.hpp"
#include "regs.hpp"

template <> void Exc_regs::set_cpu_ctrl0<Vmcb> (uint32 val)
{
    unsigned const msk = !!cr0_msk<Vmcb>() << 0 | !!cr4_msk<Vmcb>() << 4;

    vmcb->npt_control  = 1;
    vmcb->intercept_cr = (msk << 16) | msk;

    vmcb->intercept_cpu[0] = val | Vmcb::force_ctrl0;
}

template <> void Exc_regs::set_cpu_ctrl1<Vmcb> (uint32 val)
{
    vmcb->intercept_cpu[1] = val | Vmcb::force_ctrl1;
}

template <> void Exc_regs::set_cpu_ctrl0<Vmcs> (uint32 val)
{
    Vmcs::write (Vmcs::CPU_EXEC_CTRL0, (val | Vmcs::ctrl_cpu[0].set) & Vmcs::ctrl_cpu[0].clr);
}

template <> void Exc_regs::set_cpu_ctrl1<Vmcs> (uint32 val)
{
    val |= Vmcs::CPU_EPT;

    Vmcs::write (Vmcs::CPU_EXEC_CTRL1, (val | Vmcs::ctrl_cpu[1].set) & Vmcs::ctrl_cpu[1].clr);
}

template <> void Exc_regs::nst_ctrl<Vmcb>()
{
    mword cr0 = get_cr0<Vmcb>();
    mword cr3 = get_cr3<Vmcb>();
    mword cr4 = get_cr4<Vmcb>();
    // nst_on = Vmcb::has_npt() && on;
    set_cr0<Vmcb> (cr0);
    set_cr3<Vmcb> (cr3);
    set_cr4<Vmcb> (cr4);

    set_cpu_ctrl0<Vmcb> (vmcb->intercept_cpu[0]);
    set_cpu_ctrl1<Vmcb> (vmcb->intercept_cpu[1]);
    set_exc<Vmcb>();
}

template <> void Exc_regs::nst_ctrl<Vmcs>()
{
    assert (Vmcs::current == vmcs);

    mword cr0 = get_cr0<Vmcs>();
    mword cr3 = get_cr3<Vmcs>();
    mword cr4 = get_cr4<Vmcs>();
    // nst_on = Vmcs::has_ept() && on;
    set_cr0<Vmcs> (cr0);
    set_cr3<Vmcs> (cr3);
    set_cr4<Vmcs> (cr4);

    set_cpu_ctrl0<Vmcs> (Vmcs::read<uint32> (Vmcs::CPU_EXEC_CTRL0));
    set_cpu_ctrl1<Vmcs> (Vmcs::read<uint32> (Vmcs::CPU_EXEC_CTRL1));
    set_exc<Vmcs>();

    Vmcs::write (Vmcs::CR0_MASK, cr0_msk<Vmcs>());
    Vmcs::write (Vmcs::CR4_MASK, cr4_msk<Vmcs>());
}

void Exc_regs::fpu_ctrl (bool on)
{
    if (Hip::hip->feature() & Hip::FEAT_VMX) {

        vmcs->make_current();

        mword cr0 = get_cr0<Vmcs>();
        fpu_on = on;
        set_cr0<Vmcs> (cr0);

        set_exc<Vmcs>();

        Vmcs::write (Vmcs::CR0_MASK, cr0_msk<Vmcs>());

    } else {

        mword cr0 = get_cr0<Vmcb>();
        fpu_on = on;
        set_cr0<Vmcb> (cr0);

        set_exc<Vmcb>();

        set_cpu_ctrl0<Vmcb> (vmcb->intercept_cpu[0]);
    }
}

template <> void Exc_regs::write_efer<Vmcb> (uint64 val)
{
    vmcb->efer = val;
}

template <> void Exc_regs::write_efer<Vmcs> (uint64 val)
{
    Vmcs::write (Vmcs::GUEST_EFER, val);

    if (val & EFER_LMA)
        Vmcs::write (Vmcs::ENT_CONTROLS, Vmcs::read<uint32> (Vmcs::ENT_CONTROLS) |  Vmcs::ENT_GUEST_64);
    else
        Vmcs::write (Vmcs::ENT_CONTROLS, Vmcs::read<uint32> (Vmcs::ENT_CONTROLS) & ~Vmcs::ENT_GUEST_64);
}
