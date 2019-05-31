/*
 * User Thread Control Block (UTCB): Architecture-Specific (ARM)
 *
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

#include "cache.hpp"
#include "cpu.hpp"
#include "gicc.hpp"
#include "gich.hpp"
#include "regs.hpp"
#include "space_gst.hpp"
#include "space_obj.hpp"
#include "utcb_arch.hpp"
#include "vmcb.hpp"

void Utcb_arch::load (Mtd_arch const m, Cpu_regs const &c)
{
    auto const &e { c.exc };
    auto const v { c.vmcb };

    if (m & Mtd_arch::Item::GPR)
        for (unsigned i { 0 }; i < sizeof (el0.gpr) / sizeof (*el0.gpr); i++)
            el0.gpr[i] = e.sys.gpr[i];

    if (m & Mtd_arch::Item::EL0_SP)
        el0.sp = e.el0.sp;

    if (m & Mtd_arch::Item::EL0_IDR) {
        el0.tpidr   = e.el0.tpidr;
        el0.tpidrro = e.el0.tpidrro;
    }

    if (m & Mtd_arch::Item::EL2_ELR_SPSR) {
        el2.elr  = e.el2.elr;
        el2.spsr = e.el2.spsr;
    }

    // Don't leak a stale FAR from another EC
    if (m & Mtd_arch::Item::EL2_ESR_FAR) {
        el2.esr = e.el2.esr;
        el2.far = (BIT64_RANGE (0x35, 0x34) | BIT64_RANGE (0x25, 0x24) | BIT64_RANGE (0x22, 0x20)) & BIT64 (e.ep()) ? e.el2.far : 0;
    }

    if (!v)
        return;

    if (m & Mtd_arch::Item::A32_SPSR) {
        a32.spsr_abt = v->a32.spsr_abt;
        a32.spsr_fiq = v->a32.spsr_fiq;
        a32.spsr_irq = v->a32.spsr_irq;
        a32.spsr_und = v->a32.spsr_und;
    }

    if (m & Mtd_arch::Item::A32_DIH) {
        a32.dacr = v->a32.dacr;
        a32.ifsr = v->a32.ifsr;
        a32.hstr = v->a32.hstr;
    }

    if (m & Mtd_arch::Item::EL1_SP)
        el1.sp = v->el1.sp;

    if (m & Mtd_arch::Item::EL1_IDR) {
        el1.tpidr      = v->el1.tpidr;
        el1.contextidr = v->el1.contextidr;
    }

    if (m & Mtd_arch::Item::EL1_ELR_SPSR) {
        el1.elr  = v->el1.elr;
        el1.spsr = v->el1.spsr;
    }

    if (m & Mtd_arch::Item::EL1_ESR_FAR) {
        el1.esr = v->el1.esr;
        el1.far = v->el1.far;
    }

    if (m & Mtd_arch::Item::EL1_AFSR) {
        el1.afsr0 = v->el1.afsr0;
        el1.afsr1 = v->el1.afsr1;
    }

    if (m & Mtd_arch::Item::EL1_TTBR) {
        el1.ttbr0 = v->el1.ttbr0;
        el1.ttbr1 = v->el1.ttbr1;
    }

    if (m & Mtd_arch::Item::EL1_TCR)
        el1.tcr = v->el1.tcr;

    if (m & Mtd_arch::Item::EL1_MAIR) {
        el1.mair  = v->el1.mair;
        el1.amair = v->el1.amair;
    }

    if (m & Mtd_arch::Item::EL1_VBAR)
        el1.vbar = v->el1.vbar;

    if (m & Mtd_arch::Item::EL1_SCTLR)
        el1.sctlr = v->el1.sctlr;

    if (m & Mtd_arch::Item::EL1_MDSCR)
        el1.mdscr = v->el1.mdscr;

    if (m & Mtd_arch::Item::EL2_HCR) {
        el2.hcr  = v->el2.hcr;
        el2.hcrx = v->el2.hcrx;
    }

    if (m & Mtd_arch::Item::EL2_IDR) {
        el2.vpidr  = v->el2.vpidr;
        el2.vmpidr = v->el2.vmpidr;
    }

    // Don't leak a stale HPFAR from another vCPU
    if (m & Mtd_arch::Item::EL2_HPFAR)
        el2.hpfar = (BIT64_RANGE (0x25, 0x24) | BIT64_RANGE (0x21, 0x20)) & BIT64 (e.ep()) ? v->el2.hpfar : 0;

    if (m & Mtd_arch::Item::TMR) {
        tmr.cntvoff   = v->tmr.cntvoff;
        tmr.cntkctl   = v->tmr.cntkctl;
        tmr.cntv_cval = v->tmr.cntv_cval;
        tmr.cntv_ctl  = v->tmr.cntv_ctl;
    }

    if (m & Mtd_arch::Item::GIC) {

        if (Gicc::mode == Gicc::Mode::REGS)                                     // GICv3 => UTCBv3
            for (unsigned i { 0 }; i < Gich::num_lr; i++)
                gic.lr[i] = v->gic.lr[i];

        else                                                                    // GICv2 => UTCBv3
            for (unsigned i { 0 }; i < Gich::num_lr; i++)
                gic.lr[i] = (v->gic.lr[i] & 0x30000000) << 34 |                 // State (2 bits)
                            (v->gic.lr[i] & 0xc0000000) << 30 |                 // HW + Grp (2 bits)
                            (v->gic.lr[i] & 0x0f800000) << 28 |                 // Priority (5 bits)
                            (v->gic.lr[i] & 0x000ffc00) << 22 |                 // pINTID (10 bits)
                            (v->gic.lr[i] & 0x000003ff);                        // vINTID (10 bits)

        for (unsigned i { 0 }; i < Gich::num_apr; i++) {
            gic.ap0r[i] = v->gic.ap0r[i];
            gic.ap1r[i] = v->gic.ap1r[i];
        }

        gic.elrsr = v->gic.elrsr;
        gic.vmcr  = v->gic.vmcr;
    }
}

/*
 * The ARM manual defines the following architectural behavior for situations
 * when an exception handler sets RES0 bits or clears RES1 bits in registers:
 * - A direct write to the bit must update a storage location associated with the bit.
 * - A read of the bit must return the value last successfully written to the bit.
 * - The value of the bit must have no effect on the operation of the PE.
 */
bool Utcb_arch::save (Mtd_arch const m, Cpu_regs &c, Space_obj const *obj) const
{
    auto &e { c.exc };
    auto v { c.vmcb };

    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::ICI)
        Cache::inst_invalidate();

    if (m & Mtd_arch::Item::GPR)
        for (unsigned i { 0 }; i < sizeof (el0.gpr) / sizeof (*el0.gpr); i++)
            e.sys.gpr[i] = el0.gpr[i];

    if (m & Mtd_arch::Item::EL0_SP)
        e.el0.sp = el0.sp;

    if (m & Mtd_arch::Item::EL0_IDR) {
        e.el0.tpidr   = el0.tpidr;
        e.el0.tpidrro = el0.tpidrro;
    }

    if (m & Mtd_arch::Item::EL2_ELR_SPSR) {
        e.el2.elr  = el2.elr;
        e.el2.spsr = v ? el2.spsr : el2.spsr & SPSR_NZCV;
    }

    // EL2_ESR_FAR state is read-only

    if (!v)
        return true;

    if (m & Mtd_arch::Item::A32_SPSR) {
        v->a32.spsr_abt = a32.spsr_abt;
        v->a32.spsr_fiq = a32.spsr_fiq;
        v->a32.spsr_irq = a32.spsr_irq;
        v->a32.spsr_und = a32.spsr_und;
    }

    if (m & Mtd_arch::Item::A32_DIH) {
        v->a32.dacr = a32.dacr;
        v->a32.ifsr = a32.ifsr;
        v->a32.hstr = a32.hstr;
    }

    if (m & Mtd_arch::Item::EL1_SP)
        v->el1.sp = el1.sp;

    if (m & Mtd_arch::Item::EL1_IDR) {
        v->el1.tpidr      = el1.tpidr;
        v->el1.contextidr = el1.contextidr;
    }

    if (m & Mtd_arch::Item::EL1_ELR_SPSR) {
        v->el1.elr  = el1.elr;
        v->el1.spsr = el1.spsr;
    }

    if (m & Mtd_arch::Item::EL1_ESR_FAR) {
        v->el1.esr = el1.esr;
        v->el1.far = el1.far;
    }

    if (m & Mtd_arch::Item::EL1_AFSR) {
        v->el1.afsr0 = el1.afsr0;
        v->el1.afsr1 = el1.afsr1;
    }

    if (m & Mtd_arch::Item::EL1_TTBR) {
        v->el1.ttbr0 = el1.ttbr0;
        v->el1.ttbr1 = el1.ttbr1;
    }

    if (m & Mtd_arch::Item::EL1_TCR)
        v->el1.tcr = el1.tcr;

    if (m & Mtd_arch::Item::EL1_MAIR) {
        v->el1.mair  = el1.mair;
        v->el1.amair = el1.amair;
    }

    if (m & Mtd_arch::Item::EL1_VBAR)
        v->el1.vbar = el1.vbar;

    if (m & Mtd_arch::Item::EL1_SCTLR)
        v->el1.sctlr = el1.sctlr;

    if (m & Mtd_arch::Item::EL1_MDSCR)
        v->el1.mdscr = el1.mdscr;

    if (m & Mtd_arch::Item::EL2_HCR) {
        v->el2.hcr  = Cpu::constrain_hcr  (el2.hcr);
        v->el2.hcrx = Cpu::constrain_hcrx (el2.hcrx);
    }

    if (m & Mtd_arch::Item::EL2_IDR) {
        v->el2.vpidr  = el2.vpidr;
        v->el2.vmpidr = el2.vmpidr;
    }

    // EL2_HPFAR state is read-only

    if (m & Mtd_arch::Item::TMR) {
        v->tmr.cntvoff   = tmr.cntvoff;
        v->tmr.cntkctl   = tmr.cntkctl;
        v->tmr.cntv_cval = tmr.cntv_cval;
        v->tmr.cntv_ctl  = tmr.cntv_ctl;
    }

    if (m & Mtd_arch::Item::GIC) {

        if (Gicc::mode == Gicc::Mode::REGS)                                     // UTCBv3 => GICv3
            for (unsigned i { 0 }; i < Gich::num_lr; i++)
                v->gic.lr[i] = gic.lr[i];

        else                                                                    // UTCBv3 => GICv2
            for (unsigned i { 0 }; i < Gich::num_lr; i++)
                v->gic.lr[i] = (gic.lr[i] & 0xc000000000000000) >> 34 |         // State (2 bits)
                               (gic.lr[i] & 0x3000000000000000) >> 30 |         // HW + Grp (2 bits)
                               (gic.lr[i] & 0x00f8000000000000) >> 28 |         // Priority (5 bits)
                               (gic.lr[i] & 0x000003ff00000000) >> 22 |         // pINTID (10 bits)
                               (gic.lr[i] & 0x00000000000003ff);                // vINTID (10 bits)

        for (unsigned i { 0 }; i < Gich::num_apr; i++) {
            v->gic.ap0r[i] = gic.ap0r[i];
            v->gic.ap1r[i] = gic.ap1r[i];
        }

        // GIC ELRSR and VMCR are read-only
    }

    if (m & Mtd_arch::Item::SPACES)
        if (EXPECT_FALSE (!assign_spaces (c, obj)))
            return false;

    if (m & (Mtd_arch::Item::EL2_HCR | Mtd_arch::Item::EL2_ELR_SPSR))
        return (v->el2.hcr & HCR_RW ? BIT (16) | BIT_RANGE (5, 4) | BIT (0) : BIT (31) | BIT (27) | BIT (23) | BIT_RANGE (19, 16)) & BIT (e.mode());

    return true;
}

bool Utcb_arch::assign_spaces (Cpu_regs &c, Space_obj const *obj) const
{
    auto const cgst { obj->lookup (sel.gst) };

    if (EXPECT_FALSE (!cgst.validate (Capability::Perm_sp::ASSIGN, Kobject::Subtype::GST)))
        return false;

    auto const gst { static_cast<Space_gst *>(cgst.obj()) };

    if (EXPECT_FALSE (gst->get_pd() != c.obj->get_pd()))
        return false;

    // FIXME: Refcount updates

    c.gst = gst;

    c.hazard.clr (Hazard::ILLEGAL);

    return true;
}
