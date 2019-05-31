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

#pragma once

#include "buddy.hpp"
#include "mtd.hpp"

class Exc_regs;
class Vmcb;

class Utcb
{
    private:
        union {
            mword mr[Mtd_user::items];

            struct {
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
                } el1;

                struct {
                    uint64  vpidr;
                    uint64  vmpidr;
                    uint64  elr;
                    uint64  spsr;
                    uint64  esr;
                    uint64  far;
                    uint64  hpfar;
                    uint64  hcr;
                } el2;

                struct {
                    uint64  cntv_cval;
                    uint64  cntv_ctl;
                    uint64  cntkctl;
                    uint64  cntvoff;
                } tmr;

                struct {
                    uint64  lr[16];
                    uint32  elrsr;
                    uint32  vmcr;
                } gic;

            } arch;
        };

    public:
        // XXX: Make private
        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }

        inline void copy (Utcb *dst, Mtd_user mtd)
        {
            for (unsigned i = 0; i < mtd.count(); i++)
                dst->mr[i] = mr[i];
        }

        void load (Mtd_arch, Exc_regs const *, Vmcb const *);
        bool save (Mtd_arch, Exc_regs *, Vmcb *) const;
};
