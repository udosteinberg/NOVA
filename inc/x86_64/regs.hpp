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
#include "selectors.hpp"
#include "svm.hpp"
#include "types.hpp"
#include "vmx.hpp"

class Sys_regs
{
    public:
        uintptr_t   r15         { 0 };
        uintptr_t   r14         { 0 };
        uintptr_t   r13         { 0 };
        uintptr_t   r12         { 0 };
        uintptr_t   r11         { 0 };
        uintptr_t   r10         { 0 };
        uintptr_t   r9          { 0 };
        uintptr_t   r8          { 0 };
        uintptr_t   rdi         { 0 };
        uintptr_t   rsi         { 0 };
        uintptr_t   rbp         { 0 };
        uintptr_t   cr2         { 0 };
        uintptr_t   rbx         { 0 };
        uintptr_t   rdx         { 0 };
        uintptr_t   rcx         { 0 };
        uintptr_t   rax         { 0 };

        inline auto &p0() const { return rdi; }
        inline auto &p1() const { return rsi; }
        inline auto &p2() const { return rdx; }
        inline auto &p3() const { return rax; }
        inline auto &p4() const { return r8;  }

        inline auto &p0()       { return rdi; }
        inline auto &p1()       { return rsi; }
        inline auto &p2()       { return rdx; }
        inline auto &ip()       { return rcx; }
        inline auto &sp()       { return r11; }

        inline unsigned flags() const { return p0() >> 4 & BIT_RANGE (3, 0); }
};

class Exc_regs : public Sys_regs
{
    public:
        union {
            struct {
                uintptr_t   gs  { 0 };
                uintptr_t   fs  { 0 };
                uintptr_t   es  { SEL_USER_DATA };
                uintptr_t   ds  { SEL_USER_DATA };
                uintptr_t   err { 0 };
                uintptr_t   vec { 0 };
                uintptr_t   rip { 0 };
                uintptr_t   cs  { SEL_USER_CODE_L };
                uintptr_t   rfl { RFL_AC | RFL_IF };
                uintptr_t   rsp { 0 };
                uintptr_t   ss  { SEL_USER_DATA };
            };
            struct {
                uintptr_t   cr0_shadow;
                uintptr_t   cr4_shadow;
                uint64      tsc_offset;
                uint32      exc_bitmap;
                bool        fpu_on;
                bool        pio_bitmap;
                bool        msr_bitmap;
            };
        };

        inline bool user() const { return cs & 3; }

        inline auto ep() const { return vec; }

        inline void set_ep (uintptr_t val) { vec = val; }
};

class Pad { uintptr_t pad; };

class alignas (16) Cpu_regs : private Pad, public Exc_regs
{
    public:
        union {
            Vmcb * const    vmcb;
            Vmcs * const    vmcs;
        };

        inline Cpu_regs()         : vmcb (nullptr) {}
        inline Cpu_regs (Vmcb *v) : vmcb (v) {}
        inline Cpu_regs (Vmcs *v) : vmcs (v) {}

    private:
        template <typename T> ALWAYS_INLINE inline auto get_g_cr0() const;
        template <typename T> ALWAYS_INLINE inline auto get_g_cr4() const;
        template <typename T> ALWAYS_INLINE inline void set_g_cr0 (uintptr_t) const;
        template <typename T> ALWAYS_INLINE inline void set_g_cr4 (uintptr_t) const;
        template <typename T> ALWAYS_INLINE inline void set_m_cr0 (uintptr_t) const;
        template <typename T> ALWAYS_INLINE inline void set_m_cr4 (uintptr_t) const;
        template <typename T> ALWAYS_INLINE inline void set_s_cr0 (uintptr_t) const;
        template <typename T> ALWAYS_INLINE inline void set_s_cr4 (uintptr_t) const;
        template <typename T> ALWAYS_INLINE inline void set_e_bmp (uint32) const;

        template <typename T>
        auto cr0_set() const
        {
            uintptr_t set = 0;

            if (!fpu_on)
                set |= CR0_TS;

            return T::fix_cr0_set | set;
        }

        template <typename T>
        auto cr0_msk() const
        {
            return T::fix_cr0_clr | cr0_set<T>();
        }

        template <typename T>
        auto cr4_set() const
        {
            uintptr_t set = 0;

            return T::fix_cr4_set | set;
        }

        template <typename T>
        auto cr4_msk() const
        {
            return T::fix_cr4_clr | cr4_set<T>();
        }

    public:
        void fpu_ctrl (bool);

        template <typename T> void tlb_invalidate() const;
        template <typename T> void set_cpu_ctrl0 (uint32);
        template <typename T> void set_cpu_ctrl1 (uint32);

        template <typename T>
        auto get_cr0() const
        {
            auto msk = cr0_msk<T>();
            return (get_g_cr0<T>() & ~msk) | (cr0_shadow & msk);
        }

        template <typename T>
        auto get_cr4() const
        {
            auto msk = cr4_msk<T>();
            return (get_g_cr4<T>() & ~msk) | (cr4_shadow & msk);
        }

        template <typename T>
        void set_cr0 (uintptr_t v)
        {
            set_s_cr0<T> (cr0_shadow = v);
            set_g_cr0<T> ((v & (~cr0_msk<T>() | CR0_PE)) | (cr0_set<T>() & ~CR0_PE));
        }

        template <typename T>
        void set_cr4 (uintptr_t v)
        {
            set_s_cr4<T> (cr4_shadow = v);
            set_g_cr4<T> ((v & ~cr4_msk<T>()) | cr4_set<T>());
        }

        template <typename T> void set_cr0_msk() { set_m_cr0<T> (cr0_msk<T>()); }
        template <typename T> void set_cr4_msk() { set_m_cr4<T> (cr4_msk<T>()); }

        template <typename T>
        void set_exc (uint32 v)
        {
            exc_bitmap = v;

            v |= BIT (EXC_AC);

            if (fpu_on)
                v &= ~BIT (EXC_NM);
            else
                v |=  BIT (EXC_NM);

            set_e_bmp<T> (v);
        }
};

template <> inline auto Cpu_regs::get_g_cr0<Vmcb>() const { return static_cast<uintptr_t>(vmcb->cr0); }
template <> inline auto Cpu_regs::get_g_cr4<Vmcb>() const { return static_cast<uintptr_t>(vmcb->cr4); }
template <> inline auto Cpu_regs::get_g_cr0<Vmcs>() const { return Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_CR0); }
template <> inline auto Cpu_regs::get_g_cr4<Vmcs>() const { return Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_CR4); }

template <> inline void Cpu_regs::set_g_cr0<Vmcb> (uintptr_t v) const   { vmcb->cr0 = v; }
template <> inline void Cpu_regs::set_g_cr4<Vmcb> (uintptr_t v) const   { vmcb->cr4 = v; }
template <> inline void Cpu_regs::set_g_cr0<Vmcs> (uintptr_t v) const   { Vmcs::write (Vmcs::Encoding::GUEST_CR0, v); }
template <> inline void Cpu_regs::set_g_cr4<Vmcs> (uintptr_t v) const   { Vmcs::write (Vmcs::Encoding::GUEST_CR4, v); }

template <> inline void Cpu_regs::set_m_cr0<Vmcb> (uintptr_t)   const   { /* FIXME */ }
template <> inline void Cpu_regs::set_m_cr4<Vmcb> (uintptr_t)   const   { /* FIXME */ }
template <> inline void Cpu_regs::set_m_cr0<Vmcs> (uintptr_t v) const   { Vmcs::write (Vmcs::Encoding::CR0_MASK, v); }
template <> inline void Cpu_regs::set_m_cr4<Vmcs> (uintptr_t v) const   { Vmcs::write (Vmcs::Encoding::CR4_MASK, v); }

template <> inline void Cpu_regs::set_s_cr0<Vmcb> (uintptr_t)   const   { /* NOP */ }
template <> inline void Cpu_regs::set_s_cr4<Vmcb> (uintptr_t)   const   { /* NOP */ }
template <> inline void Cpu_regs::set_s_cr0<Vmcs> (uintptr_t v) const   { Vmcs::write (Vmcs::Encoding::CR0_READ_SHADOW, v); }
template <> inline void Cpu_regs::set_s_cr4<Vmcs> (uintptr_t v) const   { Vmcs::write (Vmcs::Encoding::CR4_READ_SHADOW, v); }

template <> inline void Cpu_regs::set_e_bmp<Vmcb> (uint32 v) const      { vmcb->intercept_exc = v; }
template <> inline void Cpu_regs::set_e_bmp<Vmcs> (uint32 v) const      { Vmcs::write (Vmcs::Encoding::BITMAP_EXC, v); }
