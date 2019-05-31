/*
 * User Thread Control Block (UTCB): Architecture-Specific (ARM)
 *
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

#pragma once

#include "mtd_arch.hpp"

class Cpu_regs;

class Utcb_arch
{
    private:
        struct {
            uint64  gpr[31];
            uint64  sp;
            uint64  tpidr;
            uint64  tpidrro;
        } el0;

        struct {
            uint32  spsr_abt;
            uint32  spsr_fiq;
            uint32  spsr_irq;
            uint32  spsr_und;
            uint32  dacr;
            uint32  ifsr;
            uint32  reserved[2];
        } a32;

        struct {
            uint64  sp;
            uint64  tpidr;
            uint64  contextidr;
            uint64  elr;
            uint64  spsr;
            uint64  esr;
            uint64  far;
            uint64  afsr0;
            uint64  afsr1;
            uint64  ttbr0;
            uint64  ttbr1;
            uint64  tcr;
            uint64  mair;
            uint64  amair;
            uint64  vbar;
            uint64  sctlr;
            uint64  mdscr;
            uint64  reserved;
        } el1;

        struct {
            uint64  hcr;
            uint64  hcrx;
            uint64  vpidr;
            uint64  vmpidr;
            uint64  elr;
            uint64  spsr;
            uint64  esr;
            uint64  far;
            uint64  hpfar;
            uint64  reserved;
        } el2;

        struct {
            uint64  cntv_cval;
            uint64  cntv_ctl;
            uint64  cntkctl;
            uint64  cntvoff;
        } tmr;

        struct {
            uint64  lr[16];
            uint32  ap0r[4];
            uint32  ap1r[4];
            uint32  elrsr;
            uint32  vmcr;
        } gic;

    public:
        void load (Mtd_arch, Cpu_regs const *);
        bool save (Mtd_arch, Cpu_regs *) const;
};
