/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "arch.hpp"
#include "atomic.hpp"
#include "hazards.hpp"
#include "types.hpp"

class Vmcb;
class Vmcs;
class Vtlb;

class Sys_regs
{
    public:
        union {
            struct {
#ifdef __x86_64__
                mword   r15;
                mword   r14;
                mword   r13;
                mword   r12;
                mword   r11;
                mword   r10;
                mword   r9;
                mword   r8;
#endif
                mword   REG(di);
                mword   REG(si);
                mword   REG(bp);
                mword   cr2;
                mword   REG(bx);
                mword   REG(dx);
                mword   REG(cx);
                mword   REG(ax);
            };
            mword gpr[2 * SIZE];
        };

        enum Status
        {
            SUCCESS,
            COM_TIM,
            COM_ABT,
            BAD_HYP,
            BAD_CAP,
            BAD_PAR,
            BAD_FTR,
            BAD_CPU,
            BAD_DEV,
        };

        ALWAYS_INLINE
        inline unsigned flags() const { return ARG_1 >> 4 & 0xf; }

        ALWAYS_INLINE
        inline void set_status (Status status) { ARG_1 = status; }

        ALWAYS_INLINE
        inline void set_pt (mword pt) { ARG_1 = pt; }

        ALWAYS_INLINE
        inline void set_ip (mword ip) { ARG_IP = ip; }

        ALWAYS_INLINE
        inline void set_sp (mword sp) { ARG_SP = sp; }
};

class Exc_regs : public Sys_regs
{
    public:
        union {
            struct {
                mword   gs;
                mword   fs;
                mword   es;
                mword   ds;
                mword   err;
                mword   vec;
                mword   REG(ip);
                mword   cs;
                mword   REG(fl);
                mword   REG(sp);
                mword   ss;
            };
            struct {
                union {
                    Vmcs *  vmcs;
                    Vmcb *  vmcb;
                };
                Vtlb *  vtlb;
                mword   cr0_shadow;
                mword   cr3_shadow;
                mword   cr4_shadow;
                mword   dst_portal;
                mword   nst_fault;
                mword   nst_error;
                uint8   nst_on;
                uint8   fpu_on;
            };
        };

    private:
        enum Mode
        {
            MODE_REAL       = 0,
            MODE_VM86       = 1,
            MODE_PROT_16    = 2,
            MODE_PROT_32    = 4,
            MODE_PROT_64    = 8,
        };

        template <typename T> ALWAYS_INLINE inline Mode mode() const;

        template <typename T> ALWAYS_INLINE inline mword get_g_cs_dl() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_flags() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_efer() const;

        template <typename T> ALWAYS_INLINE inline mword get_g_cr0() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr2() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr3() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr4() const;

        template <typename T> ALWAYS_INLINE inline void set_g_cr0 (mword) const;
        template <typename T> ALWAYS_INLINE inline void set_g_cr2 (mword);
        template <typename T> ALWAYS_INLINE inline void set_g_cr3 (mword) const;
        template <typename T> ALWAYS_INLINE inline void set_g_cr4 (mword) const;

        template <typename T> ALWAYS_INLINE inline void set_e_bmp (uint32) const;
        template <typename T> ALWAYS_INLINE inline void set_s_cr0 (mword);
        template <typename T> ALWAYS_INLINE inline void set_s_cr4 (mword);

        template <typename T> ALWAYS_INLINE inline mword cr0_set() const;
        template <typename T> ALWAYS_INLINE inline mword cr0_msk() const;
        template <typename T> ALWAYS_INLINE inline mword cr4_set() const;
        template <typename T> ALWAYS_INLINE inline mword cr4_msk() const;

        template <typename T> ALWAYS_INLINE inline mword get_cr0() const;
        template <typename T> ALWAYS_INLINE inline mword get_cr3() const;
        template <typename T> ALWAYS_INLINE inline mword get_cr4() const;

        template <typename T> ALWAYS_INLINE inline void set_cr0 (mword);
        template <typename T> ALWAYS_INLINE inline void set_cr3 (mword);
        template <typename T> ALWAYS_INLINE inline void set_cr4 (mword);

        template <typename T> ALWAYS_INLINE inline void set_exc() const;

    public:
        ALWAYS_INLINE
        inline bool user() const { return cs & 3; }

        void fpu_ctrl (bool);

        void svm_update_shadows();

        void svm_set_cpu_ctrl0 (mword);
        void svm_set_cpu_ctrl1 (mword);
        void vmx_set_cpu_ctrl0 (mword);
        void vmx_set_cpu_ctrl1 (mword);

        mword svm_read_gpr (unsigned);
        mword vmx_read_gpr (unsigned);

        void svm_write_gpr (unsigned, mword);
        void vmx_write_gpr (unsigned, mword);

        template <typename T> void nst_ctrl (bool = T::has_urg());

        template <typename T> void tlb_flush (bool) const;
        template <typename T> void tlb_flush (mword) const;

        template <typename T> mword read_cr (unsigned) const;
        template <typename T> void write_cr (unsigned, mword);

        template <typename T> void write_efer (mword);

        template <typename T> mword linear_address (mword) const;
};

class Cpu_regs : public Exc_regs
{
    private:
        mword   hzd;

    public:
        uint64  tsc_offset;
        mword   mtd;

        ALWAYS_INLINE
        inline mword hazard() const { return hzd; }

        ALWAYS_INLINE
        inline void set_hazard (mword h) { Atomic::set_mask (hzd, h); }

        ALWAYS_INLINE
        inline void clr_hazard (mword h) { Atomic::clr_mask (hzd, h); }

        ALWAYS_INLINE
        inline void add_tsc_offset (uint64 tsc)
        {
            tsc_offset += tsc;
            set_hazard (HZD_TSC);
        }
};
