/*
 * Register File
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "regs.h"
#include "vmx.h"
#include "vtlb.h"

mword Exc_regs::cr0_set() const
{
    mword msk = 0;

    if (!ept_on)
        msk |= Cpu::CR0_PG | Cpu::CR0_WP | Cpu::CR0_PE;
    if (!fpu_on)
        msk |= Cpu::CR0_TS;

    return Vmcs::fix_cr0.set | msk;
}

mword Exc_regs::cr4_set() const
{
    mword msk = 0;

    if (!ept_on)
        msk |= Cpu::CR4_PSE;

    return Vmcs::fix_cr4.set | msk;
}

mword Exc_regs::cr0_msk() const
{
    return Vmcs::fix_cr0.clr | cr0_set();
}

mword Exc_regs::cr4_msk() const
{
    mword msk = 0;

    if (!ept_on)
        msk |= Cpu::CR4_PGE | Cpu::CR4_PAE;

    return Vmcs::fix_cr4.clr | cr4_set() | msk;
}

mword Exc_regs::get_cr0() const
{
    mword msk = cr0_msk();

    return (Vmcs::read (Vmcs::GUEST_CR0) & ~msk) | (cr0_shadow & msk);
}

mword Exc_regs::get_cr3() const
{
    return ept_on ? Vmcs::read (Vmcs::GUEST_CR3) : cr3_shadow;
}

mword Exc_regs::get_cr4() const
{
    mword msk = cr4_msk();

    return (Vmcs::read (Vmcs::GUEST_CR4) & ~msk) | (cr4_shadow & msk);
}

void Exc_regs::set_cr0 (mword val)
{
    Vmcs::write (Vmcs::GUEST_CR0, (val & (~cr0_msk() | Cpu::CR0_PE)) | (cr0_set() & ~Cpu::CR0_PE));
    Vmcs::write (Vmcs::CR0_READ_SHADOW, cr0_shadow = val);
}

void Exc_regs::set_cr3 (mword val)
{
    if (ept_on)
        Vmcs::write (Vmcs::GUEST_CR3, val);
    else
        cr3_shadow = val;
}

void Exc_regs::set_cr4 (mword val)
{
    Vmcs::write (Vmcs::GUEST_CR4, (val & ~cr4_msk()) | cr4_set());
    Vmcs::write (Vmcs::CR4_READ_SHADOW, cr4_shadow = val);
}

void Exc_regs::set_exc_bitmap() const
{
    unsigned msk = 1ul << Cpu::EXC_AC;

    if (!ept_on)
        msk |= 1ul << Cpu::EXC_PF;
    if (!fpu_on)
        msk |= 1ul << Cpu::EXC_NM;

    Vmcs::write (Vmcs::EXC_BITMAP, msk);
}

void Exc_regs::set_cpu_ctrl0 (mword val)
{
    unsigned const msk = Vmcs::CPU_INVLPG | Vmcs::CPU_CR3_LOAD | Vmcs::CPU_CR3_STORE;

    if (ept_on)
        val &= ~msk;
    else
        val |= msk;

    val |= Vmcs::ctrl_cpu[0].set;
    val &= Vmcs::ctrl_cpu[0].clr;

    Vmcs::write (Vmcs::CPU_EXEC_CTRL0, val);
}

void Exc_regs::set_cpu_ctrl1 (mword val)
{
    unsigned const msk = Vmcs::CPU_EPT;

    if (ept_on)
        val |= msk;
    else
        val &= ~msk;

    val |= Vmcs::ctrl_cpu[1].set;
    val &= Vmcs::ctrl_cpu[1].clr;

    Vmcs::write (Vmcs::CPU_EXEC_CTRL1, val);
}

void Exc_regs::ept_ctrl (bool on)
{
    assert (Vmcs::current == vmcs);

    mword cr0 = get_cr0();
    mword cr3 = get_cr3();
    mword cr4 = get_cr4();
    ept_on = on;
    set_cr0 (cr0);
    set_cr3 (cr3);
    set_cr4 (cr4);

    set_cpu_ctrl0 (Vmcs::read (Vmcs::CPU_EXEC_CTRL0));
    set_cpu_ctrl1 (Vmcs::read (Vmcs::CPU_EXEC_CTRL1));

    set_exc_bitmap();
    Vmcs::write (Vmcs::CR0_MASK, cr0_msk());
    Vmcs::write (Vmcs::CR4_MASK, cr4_msk());

    if (!on)
        Vmcs::write (Vmcs::GUEST_CR3, Buddy::ptr_to_phys (vtlb));
}

void Exc_regs::fpu_ctrl (bool on)
{
    // Can be called on the VMCS of the FPU owner while it's not loaded
    vmcs->make_current();

    mword cr0 = get_cr0();
    fpu_on = on;
    set_cr0 (cr0);

    set_exc_bitmap();
    Vmcs::write (Vmcs::CR0_MASK, cr0_msk());
}

mword Exc_regs::read_gpr (unsigned reg)
{
    if (EXPECT_FALSE (reg == 4))
        return Vmcs::read (Vmcs::GUEST_RSP);
    else
        return gpr[7 - reg];
}

void Exc_regs::write_gpr (unsigned reg, mword val)
{
    if (EXPECT_FALSE (reg == 4))
        Vmcs::write (Vmcs::GUEST_RSP, val);
    else
        gpr[7 - reg] = val;
}

mword Exc_regs::read_cr (unsigned cr)
{
    switch (cr) {
        case 2:
            return cr2;
        case 3:
            return get_cr3();
        case 0:
            return get_cr0();
        case 4:
            return get_cr4();
        default:
            UNREACHED;
    }
}

void Exc_regs::write_cr (unsigned cr, mword val)
{
    mword toggled;

    switch (cr) {

        case 2:
            cr2 = val;
            break;

        case 3:
            vtlb->flush (false, Vmcs::vpid());
            set_cr3 (val);
            break;

        case 0:
            toggled = get_cr0() ^ val;

            if (toggled & (Cpu::CR0_PG | Cpu::CR0_PE))
                vtlb->flush (false, Vmcs::vpid());

            if (Vmcs::has_ept() && toggled & Cpu::CR0_PG)
                ept_ctrl (val & Cpu::CR0_PG);

            set_cr0 (val);
            break;

        case 4:
            toggled = get_cr4() ^ val;

            if (toggled & (Cpu::CR4_PGE | Cpu::CR4_PAE | Cpu::CR4_PSE))
                vtlb->flush (toggled & Cpu::CR4_PGE, Vmcs::vpid());

            set_cr4 (val);
            break;

        default:
            UNREACHED;
    }
}
