/*
 * User Thread Control Block (UTCB): Architecture-Specific (x86)
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

#pragma once

#include "mtd_arch.hpp"

class Cpu_regs;
class Exc_regs;
class Space_obj;

class Utcb_segment final
{
    public:
        uint16_t    sel, ar;
        uint32_t    limit;
        uint64_t    base;

        void set_vmx (uint16_t s, uint64_t b, uint32_t l, uint32_t a)
        {
            sel   = s;
            ar    = static_cast<uint16_t>((a >> 4 & 0x1f00) | (a & 0xff));
            limit = l;
            base  = b;
        }
};

class Utcb_arch final
{
    private:
        uint64_t        rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
        uint64_t        r8,  r9,  r10, r11, r12, r13, r14, r15;
        uint64_t        rfl, rip;
        uint32_t        inst_len, inst_info, intr_state, actv_state;
        uint64_t        qual[3], reserved;
        uint32_t        ctrl_pri, ctrl_sec;
        uint64_t        ctrl_ter;
        uint64_t        intcpt_cr0, intcpt_cr4;
        uint32_t        intcpt_exc, pfe_mask, pfe_match, tpr_threshold;
        uint32_t        intr_info, intr_errc, vect_info, vect_errc;
        Utcb_segment    cs, ss, ds, es, fs, gs, tr, ld, gd, id;
        uint64_t        pdpte[4];
        uint64_t        cr0, cr2, cr3, cr4, cr8, dr7, xcr0, xss;
        uint64_t        apic_base, sysenter_cs, sysenter_esp, sysenter_eip;
        uint64_t        pat, efer, star, lstar, fmask, kernel_gs_base, tsc_aux;

        struct {
            uint64_t    gst;
            uint64_t    pio;
            uint64_t    msr;
        } sel;

    public:
        void load_exc (Mtd_arch const, Exc_regs const &);
        void load_vmx (Mtd_arch const, Cpu_regs const &);
        void load_svm (Mtd_arch const, Cpu_regs const &);
        bool save_exc (Mtd_arch const, Exc_regs &) const;
        bool save_vmx (Mtd_arch const, Cpu_regs &) const;
        bool save_svm (Mtd_arch const, Cpu_regs &) const;
};

static_assert (__is_standard_layout (Utcb_arch) && sizeof (Utcb_arch) == 0x270);
