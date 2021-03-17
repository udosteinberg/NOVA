/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "arch.hpp"
#include "hazards.hpp"
#include "svm.hpp"
#include "types.hpp"
#include "vmx.hpp"

class Sys_regs
{
    public:
        mword   r15         { 0 };
        mword   r14         { 0 };
        mword   r13         { 0 };
        mword   r12         { 0 };
        mword   r11         { 0 };
        mword   r10         { 0 };
        mword   r9          { 0 };
        mword   r8          { 0 };
        mword   rdi         { 0 };
        mword   rsi         { 0 };
        mword   rbp         { 0 };
        mword   cr2         { 0 };
        mword   rbx         { 0 };
        mword   rdx         { 0 };
        mword   rcx         { 0 };
        mword   rax         { 0 };

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
                mword   rip;
                mword   cs;
                mword   rfl;
                mword   rsp;
                mword   ss;
            };
            struct {
                union {
                    Vmcs *  vmcs;
                    Vmcb *  vmcb;
                };
                mword   reserved1;
                mword   reserved2;
                mword   cr0_shadow;
                mword   cr4_shadow;
                mword   dst_portal;
                uint8   fpu_on;
            };
        };

    private:
        template <typename T> ALWAYS_INLINE inline mword get_g_cr0() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr2() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr3() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr4() const;
        template <typename T> ALWAYS_INLINE inline void  set_g_cr0 (mword) const;
        template <typename T> ALWAYS_INLINE inline void  set_g_cr2 (mword);
        template <typename T> ALWAYS_INLINE inline void  set_g_cr3 (mword) const;
        template <typename T> ALWAYS_INLINE inline void  set_g_cr4 (mword) const;
        template <typename T> ALWAYS_INLINE inline void  set_s_cr0 (mword);
        template <typename T> ALWAYS_INLINE inline void  set_s_cr4 (mword);
        template <typename T> ALWAYS_INLINE inline void  set_e_bmp (uint32) const;

    protected:
        template <typename T> ALWAYS_INLINE inline uint64 get_g_efer() const;

        template <typename T>
        mword cr0_set() const
        {
            mword set = 0;

            if (!fpu_on)
                set |= CR0_TS;

            return T::fix_cr0_set | set;
        }

        template <typename T>
        mword cr0_msk() const
        {
            return T::fix_cr0_clr | cr0_set<T>();
        }

        template <typename T>
        mword cr4_set() const
        {
            mword set = 0;

            return T::fix_cr4_set | set;
        }

        template <typename T>
        mword cr4_msk() const
        {
            return T::fix_cr4_clr | cr4_set<T>();
        }

    public:
        ALWAYS_INLINE
        inline bool user() const { return cs & 3; }

        void fpu_ctrl (bool);

        template <typename T> void set_cpu_ctrl0 (uint32);
        template <typename T> void set_cpu_ctrl1 (uint32);

        template <typename T> void nst_ctrl();

        template <typename T>
        mword get_cr0() const
        {
            mword msk = cr0_msk<T>();
            return (get_g_cr0<T>() & ~msk) | (cr0_shadow & msk);
        }

        template <typename T>
        mword get_cr2() const
        {
            return get_g_cr2<T>();
        }

        template <typename T>
        mword get_cr3() const
        {
            return get_g_cr3<T>();
        }

        template <typename T>
        mword get_cr4() const
        {
            mword msk = cr4_msk<T>();
            return (get_g_cr4<T>() & ~msk) | (cr4_shadow & msk);
        }

        template <typename T>
        void set_cr0 (mword v)
        {
            set_g_cr0<T> ((v & (~cr0_msk<T>() | CR0_PE)) | (cr0_set<T>() & ~CR0_PE));
            set_s_cr0<T> (v);
        }

        template <typename T>
        void set_cr2 (mword v)
        {
            set_g_cr2<T> (v);
        }

        template <typename T>
        void set_cr3 (mword v)
        {
            set_g_cr3<T> (v);
        }

        template <typename T>
        void set_cr4 (mword v)
        {
            set_g_cr4<T> ((v & ~cr4_msk<T>()) | cr4_set<T>());
            set_s_cr4<T> (v);
        }

        template <typename T>
        void set_exc()
        {
            uint32 val = BIT (EXC_AC);

            if (!fpu_on)
                val |= BIT (EXC_NM);

            set_e_bmp<T> (val);
        }

        template <typename T> void write_efer (uint64);
};

class Cpu_regs : public Exc_regs
{
    public:
        uint64  tsc_offset;
        mword   mtd;
};

template <> inline uint64 Exc_regs::get_g_efer<Vmcb>()         const { return vmcb->efer; }
template <> inline uint64 Exc_regs::get_g_efer<Vmcs>()         const { return Vmcs::read<uint64> (Vmcs::Encoding::GUEST_EFER); }

template <> inline mword  Exc_regs::get_g_cr0<Vmcb>()          const { return static_cast<mword>(vmcb->cr0); }
template <> inline mword  Exc_regs::get_g_cr2<Vmcb>()          const { return static_cast<mword>(vmcb->cr2); }
template <> inline mword  Exc_regs::get_g_cr3<Vmcb>()          const { return static_cast<mword>(vmcb->cr3); }
template <> inline mword  Exc_regs::get_g_cr4<Vmcb>()          const { return static_cast<mword>(vmcb->cr4); }
template <> inline mword  Exc_regs::get_g_cr0<Vmcs>()          const { return Vmcs::read<mword>  (Vmcs::Encoding::GUEST_CR0); }
template <> inline mword  Exc_regs::get_g_cr2<Vmcs>()          const { return cr2; }
template <> inline mword  Exc_regs::get_g_cr3<Vmcs>()          const { return Vmcs::read<mword>  (Vmcs::Encoding::GUEST_CR3); }
template <> inline mword  Exc_regs::get_g_cr4<Vmcs>()          const { return Vmcs::read<mword>  (Vmcs::Encoding::GUEST_CR4); }

template <> inline void   Exc_regs::set_g_cr0<Vmcb> (mword v)  const { vmcb->cr0 = v; }
template <> inline void   Exc_regs::set_g_cr2<Vmcb> (mword v)        { vmcb->cr2 = v; }
template <> inline void   Exc_regs::set_g_cr3<Vmcb> (mword v)  const { vmcb->cr3 = v; }
template <> inline void   Exc_regs::set_g_cr4<Vmcb> (mword v)  const { vmcb->cr4 = v; }
template <> inline void   Exc_regs::set_g_cr0<Vmcs> (mword v)  const { Vmcs::write (Vmcs::Encoding::GUEST_CR0, v); }
template <> inline void   Exc_regs::set_g_cr2<Vmcs> (mword v)        { cr2 = v; }
template <> inline void   Exc_regs::set_g_cr3<Vmcs> (mword v)  const { Vmcs::write (Vmcs::Encoding::GUEST_CR3, v); }
template <> inline void   Exc_regs::set_g_cr4<Vmcs> (mword v)  const { Vmcs::write (Vmcs::Encoding::GUEST_CR4, v); }

template <> inline void   Exc_regs::set_s_cr0<Vmcb> (mword v)        { cr0_shadow = v; }
template <> inline void   Exc_regs::set_s_cr4<Vmcb> (mword v)        { cr4_shadow = v; }
template <> inline void   Exc_regs::set_s_cr0<Vmcs> (mword v)        { Vmcs::write (Vmcs::Encoding::CR0_READ_SHADOW, cr0_shadow = v); }
template <> inline void   Exc_regs::set_s_cr4<Vmcs> (mword v)        { Vmcs::write (Vmcs::Encoding::CR4_READ_SHADOW, cr4_shadow = v); }

template <> inline void   Exc_regs::set_e_bmp<Vmcb> (uint32 v) const { vmcb->intercept_exc = v; }
template <> inline void   Exc_regs::set_e_bmp<Vmcs> (uint32 v) const { Vmcs::write (Vmcs::Encoding::BITMAP_EXC, v); }
