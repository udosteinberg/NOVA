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

#pragma once

#include "atomic.h"
#include "compiler.h"
#include "hazards.h"
#include "types.h"

class Vmcb;
class Vmcs;
class Vtlb;

class Sys_regs
{
    protected:
        union {
            struct {
                mword   edi;
                mword   esi;
                mword   ebp;
                mword   cr2;
                mword   ebx;
                mword   edx;
                mword   ecx;
                mword   eax;
            };
            mword gpr[];
        };

    public:
        enum Status
        {
            SUCCESS,
            TIMEOUT,
            BAD_SYS,
            BAD_CAP,
            BAD_MEM,
            BAD_FTR,
            BAD_CPU,
            BAD_DEV,
        };

        ALWAYS_INLINE
        inline unsigned flags() const { return eax >> 4 & 0xf; }

        ALWAYS_INLINE
        inline void set_status (Status status) { eax = status; }

        ALWAYS_INLINE
        inline void set_pt (mword pt) { eax = pt; }

        ALWAYS_INLINE
        inline void set_ip (mword ip) { edx = ip; }
};

class Exc_regs : public Sys_regs
{
    protected:
        union {
            struct {
                mword   gs;
                mword   fs;
                mword   es;
                mword   ds;
                mword   err;
                mword   vec;
                mword   eip;
                mword   cs;
                mword   efl;
                mword   esp;
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
