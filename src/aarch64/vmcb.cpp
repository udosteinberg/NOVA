/*
 * Virtual Machine Control Block (VMCB)
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

#include "cpu.hpp"
#include "fpu.hpp"
#include "gich.hpp"
#include "timer.hpp"
#include "vmcb.hpp"

Vmcb const *Vmcb::current { nullptr };

void Vmcb::init()
{
    asm volatile ("msr mdcr_el2,        %0" : : "r" (Cpu::mdcr));
    asm volatile ("msr osdlr_el1,       xzr");
    asm volatile ("msr oslar_el1,       xzr");
    asm volatile ("msr cnthctl_el2,     xzr");

    Fpu::disable();

    load_hst();
}

void Vmcb::load_hst()
{
    current = nullptr;

    asm volatile ("msr cpacr_el1,       %0" : : "r" (BIT64_RANGE (21, 20)));
    asm volatile ("msr mdscr_el1,       xzr");
    asm volatile ("msr sctlr_el1,       %0" : : "r" (SCTLR_A64_UCI | SCTLR_A64_UCT | SCTLR_A64_DZE | SCTLR_ALL_I | SCTLR_A64_SA0 | SCTLR_A64_SA | SCTLR_ALL_C | SCTLR_ALL_A | SCTLR_ALL_M));

    asm volatile ("msr hcr_el2,         %0" : : "r" (HCR_RW | HCR_TGE | HCR_DC | HCR_VM));  // TGE entails AMO, IMO, FMO

    asm volatile ("msr cntvoff_el2,     %0" : : "r" (Timer::syst_to_phys (0)));
    asm volatile ("msr cntkctl_el1,     %0" : : "r" (BIT64 (1)));
    asm volatile ("msr cntv_ctl_el0,    xzr");

    Gich::disable();
}

void Vmcb::load_gst() const
{
    current = this;

    asm volatile ("msr afsr0_el1,       %0" : : "r" (el1.afsr0));
    asm volatile ("msr afsr1_el1,       %0" : : "r" (el1.afsr1));
    asm volatile ("msr amair_el1,       %0" : : "r" (el1.amair));
    asm volatile ("msr contextidr_el1,  %0" : : "r" (el1.contextidr));
    asm volatile ("msr cpacr_el1,       %0" : : "r" (el1.cpacr));
    asm volatile ("msr csselr_el1,      %0" : : "r" (el1.csselr));
    asm volatile ("msr elr_el1,         %0" : : "r" (el1.elr));
    asm volatile ("msr esr_el1,         %0" : : "r" (el1.esr));
    asm volatile ("msr far_el1,         %0" : : "r" (el1.far));
    asm volatile ("msr mair_el1,        %0" : : "r" (el1.mair));
    asm volatile ("msr mdscr_el1,       %0" : : "r" (el1.mdscr));
    asm volatile ("msr par_el1,         %0" : : "r" (el1.par));
    asm volatile ("msr sctlr_el1,       %0" : : "r" (el1.sctlr));
    asm volatile ("msr sp_el1,          %0" : : "r" (el1.sp));
    asm volatile ("msr spsr_el1,        %0" : : "r" (el1.spsr));
    asm volatile ("msr tcr_el1,         %0" : : "r" (el1.tcr));
    asm volatile ("msr tpidr_el1,       %0" : : "r" (el1.tpidr));
    asm volatile ("msr ttbr0_el1,       %0" : : "r" (el1.ttbr0));
    asm volatile ("msr ttbr1_el1,       %0" : : "r" (el1.ttbr1));
    asm volatile ("msr vbar_el1,        %0" : : "r" (el1.vbar));

    asm volatile ("msr hcr_el2,         %0" : : "r" (el2.hcr));
//  asm volatile ("msr vdisr_el2,       %0" : : "r" (el2.vdisr));   // RAS
    asm volatile ("msr vmpidr_el2,      %0" : : "r" (el2.vmpidr));
    asm volatile ("msr vpidr_el2,       %0" : : "r" (el2.vpidr));

    if (EXPECT_FALSE (!(el2.hcr & HCR_RW))) {
        asm volatile ("msr dacr32_el2, %x0" : : "r" (a32.dacr));
        asm volatile ("msr ifsr32_el2, %x0" : : "r" (a32.ifsr));
        asm volatile ("msr hstr_el2,   %x0" : : "r" (a32.hstr));
        asm volatile ("msr spsr_abt,   %x0" : : "r" (a32.spsr_abt));
        asm volatile ("msr spsr_fiq,   %x0" : : "r" (a32.spsr_fiq));
        asm volatile ("msr spsr_irq,   %x0" : : "r" (a32.spsr_irq));
        asm volatile ("msr spsr_und,   %x0" : : "r" (a32.spsr_und));
    }

    // Load timer interrupt state
    load_tmr();

    // Load timer register state
    asm volatile ("msr cntvoff_el2,     %0" : : "r" (Timer::syst_to_phys (tmr.cntvoff)));
    asm volatile ("msr cntkctl_el1,     %0" : : "r" (tmr.cntkctl));
    asm volatile ("msr cntv_cval_el0,   %0" : : "r" (tmr.cntv_cval));
    asm volatile ("msr cntv_ctl_el0,    %0" : : "r" (tmr.cntv_ctl));

    Gich::load (gic.lr, gic.ap0r, gic.ap1r, gic.hcr, gic.vmcr);
}

void Vmcb::save_gst()
{
    asm volatile ("mrs %0, afsr0_el1"       : "=r" (el1.afsr0));
    asm volatile ("mrs %0, afsr1_el1"       : "=r" (el1.afsr1));
    asm volatile ("mrs %0, amair_el1"       : "=r" (el1.amair));
    asm volatile ("mrs %0, contextidr_el1"  : "=r" (el1.contextidr));
    asm volatile ("mrs %0, cpacr_el1"       : "=r" (el1.cpacr));
    asm volatile ("mrs %0, csselr_el1"      : "=r" (el1.csselr));
    asm volatile ("mrs %0, elr_el1"         : "=r" (el1.elr));
    asm volatile ("mrs %0, esr_el1"         : "=r" (el1.esr));
    asm volatile ("mrs %0, far_el1"         : "=r" (el1.far));
    asm volatile ("mrs %0, mair_el1"        : "=r" (el1.mair));
    // mdscr_el1 is trapped to the VMM by MDCR_TDE
    asm volatile ("mrs %0, par_el1"         : "=r" (el1.par));
    asm volatile ("mrs %0, sctlr_el1"       : "=r" (el1.sctlr));
    asm volatile ("mrs %0, sp_el1"          : "=r" (el1.sp));
    asm volatile ("mrs %0, spsr_el1"        : "=r" (el1.spsr));
    asm volatile ("mrs %0, tcr_el1"         : "=r" (el1.tcr));
    asm volatile ("mrs %0, tpidr_el1"       : "=r" (el1.tpidr));
    asm volatile ("mrs %0, ttbr0_el1"       : "=r" (el1.ttbr0));
    asm volatile ("mrs %0, ttbr1_el1"       : "=r" (el1.ttbr1));
    asm volatile ("mrs %0, vbar_el1"        : "=r" (el1.vbar));

    asm volatile ("mrs %0, hpfar_el2"       : "=r" (el2.hpfar));
//  asm volatile ("mrs %0, vdisr_el2"       : "=r" (el2.vdisr));    // RAS

    if (EXPECT_FALSE (!(el2.hcr & HCR_RW))) {
        asm volatile ("mrs %x0, dacr32_el2" : "=r" (a32.dacr));
        asm volatile ("mrs %x0, ifsr32_el2" : "=r" (a32.ifsr));
        asm volatile ("mrs %x0, hstr_el2"   : "=r" (a32.hstr));
        asm volatile ("mrs %x0, spsr_abt"   : "=r" (a32.spsr_abt));
        asm volatile ("mrs %x0, spsr_fiq"   : "=r" (a32.spsr_fiq));
        asm volatile ("mrs %x0, spsr_irq"   : "=r" (a32.spsr_irq));
        asm volatile ("mrs %x0, spsr_und"   : "=r" (a32.spsr_und));
    }

    // Save timer interrupt state
    save_tmr();

    // Save timer register state
    asm volatile ("mrs %0, cntkctl_el1"     : "=r" (tmr.cntkctl));
    asm volatile ("mrs %0, cntv_cval_el0"   : "=r" (tmr.cntv_cval));
    asm volatile ("mrs %0, cntv_ctl_el0"    : "=r" (tmr.cntv_ctl));

    Gich::save (gic.lr, gic.ap0r, gic.ap1r, gic.hcr, gic.vmcr, gic.elrsr);
}
