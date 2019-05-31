/*
 * User Thread Control Block (UTCB)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "gicc.hpp"
#include "gich.hpp"
#include "mtd.hpp"
#include "regs.hpp"
#include "utcb.hpp"
#include "vmcb.hpp"

void Utcb::load (Mtd_arch mtd, Exc_regs const *r, Vmcb const *v)
{
    mword m = mtd.val();

    if (m & Mtd_arch::Item::GPR)
        for (unsigned i = 0; i < 31; i++)
            arch.el0.gpr[i] = r->r[i];

    if (m & Mtd_arch::Item::EL0_SP)
        arch.el0.sp = r->el0_sp;

    if (m & Mtd_arch::Item::EL0_IDR) {
        arch.el0.tpidr   = r->el0_tpidr;
        arch.el0.tpidrro = r->el0_tpidrro;
    }

    if (m & Mtd_arch::Item::EL2_ELR_SPSR) {
        arch.el2.elr  = r->el2_elr;
        arch.el2.spsr = r->el2_spsr;
    }

    // Don't leak a stale FAR from another EC
    if (m & Mtd_arch::Item::EL2_ESR_FAR) {
        arch.el2.esr = r->el2_esr;
        arch.el2.far = (BIT64_RANGE (0x35, 0x34) | BIT64_RANGE (0x25, 0x24) | BIT64_RANGE (0x22, 0x20)) & BIT64 (r->ep()) ? r->el2_far : 0;
    }

    if (!v)
        return;

    // Don't leak a stale HPFAR from another vCPU
    if (m & Mtd_arch::Item::EL2_HPFAR)
        arch.el2.hpfar = (BIT64_RANGE (0x25, 0x24) | BIT64_RANGE (0x21, 0x20)) & BIT64 (r->ep()) ? v->el2.hpfar : 0;

    if (m & Mtd_arch::Item::A32_SPSR) {
        arch.a32.spsr_abt = v->a32.spsr_abt;
        arch.a32.spsr_fiq = v->a32.spsr_fiq;
        arch.a32.spsr_irq = v->a32.spsr_irq;
        arch.a32.spsr_und = v->a32.spsr_und;
    }

    if (m & Mtd_arch::Item::A32_DACR_IFSR) {
        arch.a32.dacr = v->a32.dacr;
        arch.a32.ifsr = v->a32.ifsr;
    }

    if (m & Mtd_arch::Item::EL1_SP)
        arch.el1.sp = v->el1.sp;

    if (m & Mtd_arch::Item::EL1_IDR) {
        arch.el1.tpidr      = v->el1.tpidr;
        arch.el1.contextidr = v->el1.contextidr;
    }

    if (m & Mtd_arch::Item::EL1_ELR_SPSR) {
        arch.el1.elr  = v->el1.elr;
        arch.el1.spsr = v->el1.spsr;
    }

    if (m & Mtd_arch::Item::EL1_ESR_FAR) {
        arch.el1.esr = v->el1.esr;
        arch.el1.far = v->el1.far;
    }

    if (m & Mtd_arch::Item::EL1_AFSR) {
        arch.el1.afsr0 = v->el1.afsr0;
        arch.el1.afsr1 = v->el1.afsr1;
    }

    if (m & Mtd_arch::Item::EL1_TTBR) {
        arch.el1.ttbr0 = v->el1.ttbr0;
        arch.el1.ttbr1 = v->el1.ttbr1;
    }

    if (m & Mtd_arch::Item::EL1_TCR)
        arch.el1.tcr = v->el1.tcr;

    if (m & Mtd_arch::Item::EL1_MAIR) {
        arch.el1.mair  = v->el1.mair;
        arch.el1.amair = v->el1.amair;
    }

    if (m & Mtd_arch::Item::EL1_VBAR)
        arch.el1.vbar = v->el1.vbar;

    if (m & Mtd_arch::Item::EL1_SCTLR)
        arch.el1.sctlr = v->el1.sctlr;

    if (m & Mtd_arch::Item::EL2_IDR) {
        arch.el2.vpidr  = v->el2.vpidr;
        arch.el2.vmpidr = v->el2.vmpidr;
    }

    if (m & Mtd_arch::Item::EL2_HCR)
        arch.el2.hcr = v->el2.hcr;

    if (m & Mtd_arch::Item::TMR) {
        arch.tmr.cntvoff   = v->tmr.cntvoff;
        arch.tmr.cntkctl   = v->tmr.cntkctl;
        arch.tmr.cntv_cval = v->tmr.cntv_cval;
        arch.tmr.cntv_ctl  = v->tmr.cntv_ctl;
    }

    if (m & Mtd_arch::Item::GIC) {

        if (Gicc::mode == Gicc::Mode::REGS)                                     // GICv3 => UTCBv3
            for (unsigned i = 0; i < Gich::num_lr; i++)
                arch.gic.lr[i] = v->gic.lr[i];

        else                                                                    // GICv2 => UTCBv3
            for (unsigned i = 0; i < Gich::num_lr; i++)
                arch.gic.lr[i] = (v->gic.lr[i] & 0x30000000) << 34 |            // State (2 bits)
                                 (v->gic.lr[i] & 0xc0000000) << 30 |            // HW + Grp (2 bits)
                                 (v->gic.lr[i] & 0x0f800000) << 28 |            // Priority (5 bits)
                                 (v->gic.lr[i] & 0x000ffc00) << 22 |            // pINTID (10 bits)
                                 (v->gic.lr[i] & 0x000003ff);                   // vINTID (10 bits)

        arch.gic.elrsr = v->gic.elrsr;
        arch.gic.vmcr  = v->gic.vmcr;
    }
}

bool Utcb::save (Mtd_arch mtd, Exc_regs *r, Vmcb *v) const
{
    mword m = mtd.val();

    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::GPR)
        for (unsigned i = 0; i < 31; i++)
            r->r[i] = arch.el0.gpr[i];

    if (m & Mtd_arch::Item::EL0_SP)
        r->el0_sp = arch.el0.sp;

    if (m & Mtd_arch::Item::EL0_IDR) {
        r->el0_tpidr   = arch.el0.tpidr;
        r->el0_tpidrro = arch.el0.tpidrro;
    }

    if (m & Mtd_arch::Item::EL2_ELR_SPSR) {
        r->el2_elr  = arch.el2.elr;
        r->el2_spsr = arch.el2.spsr & ~(arch.el2.spsr & PSTATE_ALL_nRW ? Cpu::spsr32_res0 : Cpu::spsr64_res0);
    }

    // EL2_ESR_FAR state is read-only

    if (!v)
        return !r->emode();

    // EL2_HPFAR state is read-only

    if (m & Mtd_arch::Item::A32_SPSR) {
        v->a32.spsr_abt = arch.a32.spsr_abt;
        v->a32.spsr_fiq = arch.a32.spsr_fiq;
        v->a32.spsr_irq = arch.a32.spsr_irq;
        v->a32.spsr_und = arch.a32.spsr_und;
    }

    if (m & Mtd_arch::Item::A32_DACR_IFSR) {
        v->a32.dacr = arch.a32.dacr;
        v->a32.ifsr = arch.a32.ifsr;
    }

    if (m & Mtd_arch::Item::EL1_SP)
        v->el1.sp = arch.el1.sp;

    if (m & Mtd_arch::Item::EL1_IDR) {
        v->el1.tpidr      = arch.el1.tpidr;
        v->el1.contextidr = arch.el1.contextidr;
    }

    if (m & Mtd_arch::Item::EL1_ELR_SPSR) {
        v->el1.elr  = arch.el1.elr;
        v->el1.spsr = arch.el1.spsr & ~(arch.el1.spsr & PSTATE_ALL_nRW ? Cpu::spsr32_res0 : Cpu::spsr64_res0);
    }

    if (m & Mtd_arch::Item::EL1_ESR_FAR) {
        v->el1.esr = arch.el1.esr;
        v->el1.far = arch.el1.far;
    }

    if (m & Mtd_arch::Item::EL1_AFSR) {
        v->el1.afsr0 = arch.el1.afsr0;
        v->el1.afsr1 = arch.el1.afsr1;
    }

    if (m & Mtd_arch::Item::EL1_TTBR) {
        v->el1.ttbr0 = arch.el1.ttbr0;
        v->el1.ttbr1 = arch.el1.ttbr1;
    }

    if (m & Mtd_arch::Item::EL1_TCR)
        v->el1.tcr = arch.el1.tcr & ~(v->el1.spsr & PSTATE_ALL_nRW ? Cpu::tcr32_res0 : Cpu::tcr64_res0);

    if (m & Mtd_arch::Item::EL1_MAIR) {
        v->el1.mair  = arch.el1.mair;
        v->el1.amair = arch.el1.amair;
    }

    if (m & Mtd_arch::Item::EL1_VBAR)
        v->el1.vbar = arch.el1.vbar;

    if (m & Mtd_arch::Item::EL1_SCTLR)
        v->el1.sctlr = (arch.el1.sctlr |  (v->el1.spsr & PSTATE_ALL_nRW ? Cpu::sctlr32_res1 : Cpu::sctlr64_res1))
                                       & ~(v->el1.spsr & PSTATE_ALL_nRW ? Cpu::sctlr32_res0 : Cpu::sctlr64_res0);

    if (m & Mtd_arch::Item::EL2_IDR) {
        v->el2.vpidr  = arch.el2.vpidr;
        v->el2.vmpidr = arch.el2.vmpidr;
    }

    if (m & Mtd_arch::Item::EL2_HCR)
        v->el2.hcr = ((arch.el2.hcr | Vmcb::hcr_hyp1) & ~Vmcb::hcr_hyp0) & ~Cpu::hcr_res0;

    if (m & Mtd_arch::Item::TMR) {
        v->tmr.cntvoff   = arch.tmr.cntvoff;
        v->tmr.cntkctl   = arch.tmr.cntkctl;
        v->tmr.cntv_cval = arch.tmr.cntv_cval;
        v->tmr.cntv_ctl  = arch.tmr.cntv_ctl;
    }

    if (m & Mtd_arch::Item::GIC) {

        if (Gicc::mode == Gicc::Mode::REGS)                                     // UTCBv3 => GICv3
            for (unsigned i = 0; i < Gich::num_lr; i++)
                v->gic.lr[i] = arch.gic.lr[i];

        else                                                                    // UTCBv3 => GICv2
            for (unsigned i = 0; i < Gich::num_lr; i++)
                v->gic.lr[i] = (arch.gic.lr[i] & 0xc000000000000000) >> 34 |    // State (2 bits)
                               (arch.gic.lr[i] & 0x3000000000000000) >> 30 |    // HW + Grp (2 bits)
                               (arch.gic.lr[i] & 0x00f8000000000000) >> 28 |    // Priority (5 bits)
                               (arch.gic.lr[i] & 0x000003ff00000000) >> 22 |    // pINTID (10 bits)
                               (arch.gic.lr[i] & 0x00000000000003ff);           // vINTID (10 bits)

        // GIC ELRSR and VMCR are read-only
    }

    if (m & (Mtd_arch::Item::EL2_HCR | Mtd_arch::Item::EL2_ELR_SPSR))
        return (v->el2.hcr & HCR_RW ? BIT_RANGE (5, 4) | BIT (0) : BIT (31) | BIT (27) | BIT (23) | BIT_RANGE (19, 16)) & BIT (r->emode());

    return true;
}
