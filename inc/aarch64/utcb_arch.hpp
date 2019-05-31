/*
 * User Thread Control Block (UTCB): Architecture-Specific (ARM)
 *
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

#pragma once

#include "mtd_arch.hpp"

class Cpu_regs;
class Space_obj;

class Utcb_arch final
{
    private:
        struct {
            uint64_t    gpr[31];
            uint64_t    sp;
            uint64_t    tpidr;
            uint64_t    tpidrro;
        } el0;

        struct {
            uint32_t    spsr_abt;
            uint32_t    spsr_fiq;
            uint32_t    spsr_irq;
            uint32_t    spsr_und;
            uint32_t    dacr;
            uint32_t    ifsr;
            uint32_t    hstr;
            uint32_t    reserved;
        } a32;

        struct {
            uint64_t    sp;
            uint64_t    tpidr;
            uint64_t    contextidr;
            uint64_t    elr;
            uint64_t    spsr;
            uint64_t    esr;
            uint64_t    far;
            uint64_t    afsr0;
            uint64_t    afsr1;
            uint64_t    ttbr0;
            uint64_t    ttbr1;
            uint64_t    tcr;
            uint64_t    mair;
            uint64_t    amair;
            uint64_t    vbar;
            uint64_t    sctlr;
            uint64_t    mdscr;
            uint64_t    reserved;
        } el1;

        struct {
            uint64_t    hcr;
            uint64_t    hcrx;
            uint64_t    vpidr;
            uint64_t    vmpidr;
            uint64_t    elr;
            uint64_t    spsr;
            uint64_t    esr;
            uint64_t    far;
            uint64_t    hpfar;
            uint64_t    reserved;
        } el2;

        struct {
            uint64_t    cntv_cval;
            uint64_t    cntv_ctl;
            uint64_t    cntkctl;
            uint64_t    cntvoff;
        } tmr;

        struct {
            uint64_t    lr[16];
            uint32_t    ap0r[4];
            uint32_t    ap1r[4];
            uint32_t    elrsr;
            uint32_t    vmcr;
            uint64_t    reserved;
        } gic;

        struct {
            uint64_t    gst;
        } sel;

        bool assign_spaces (Cpu_regs &, Space_obj const *) const;

    public:
        void load (Mtd_arch const, Cpu_regs const &);
        bool save (Mtd_arch const, Cpu_regs &, Space_obj const *) const;
};

static_assert (__is_standard_layout (Utcb_arch) && sizeof (Utcb_arch) == 0x2e8);
