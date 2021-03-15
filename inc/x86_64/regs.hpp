/*
 * Register File
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

#include "arch.hpp"
#include "hazard.hpp"
#include "selectors.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"
#include "svm.hpp"
#include "types.hpp"
#include "vmx.hpp"

struct Sys_regs
{
    uintptr_t   rax { 0 };
    uintptr_t   rcx { 0 };
    uintptr_t   rdx { 0 };
    uintptr_t   rbx { 0 };
    uintptr_t   rbp { 0 };
    uintptr_t   rsi { 0 };
    uintptr_t   rdi { 0 };
    uintptr_t   r8  { 0 };
    uintptr_t   r9  { 0 };
    uintptr_t   r10 { 0 };
    uintptr_t   r11 { 0 };
    uintptr_t   r12 { 0 };
    uintptr_t   r13 { 0 };
    uintptr_t   r14 { 0 };
    uintptr_t   r15 { 0 };
};

static_assert (__is_standard_layout (Sys_regs) && sizeof (Sys_regs) == __SIZEOF_POINTER__ * 15);

struct Exc_regs
{
    Sys_regs    sys;
    uintptr_t   cr2 { 0 };

    union {
        struct {
            uintptr_t   err { 0 };
            uintptr_t   vec { 0 };
            uintptr_t   rip { 0 };
            uintptr_t   cs  { SEL_USER_CODE };
            uintptr_t   rfl { RFL_AC | RFL_IF | RFL_1 };
            uintptr_t   rsp { 0 };
            uintptr_t   ss  { SEL_USER_DATA };
        };
        struct {
            uint64_t    offset_tsc;
            uintptr_t   reserved;
            uintptr_t   shadow_cr0;
            uintptr_t   shadow_cr4;
            uintptr_t   intcpt_cr0;
            uintptr_t   intcpt_cr4;
            uint32_t    intcpt_exc;
            bool        fpu_on;
        };
    };

    Exc_regs() {}       // Default constructor is non-trivial due to union

    inline auto &ip() { return sys.rcx; }
    inline auto &sp() { return sys.r11; }

    inline bool user() const { return cs & 3; }

    inline auto ep() const { return vec; }

    inline void set_ep (uintptr_t val) { vec = val; }
};

static_assert (__is_standard_layout (Exc_regs) && sizeof (Exc_regs) == __SIZEOF_POINTER__ * 23);

class alignas (16) Cpu_regs final
{
    private:
        template <typename T> inline auto set_cr0() const { return T::fix_cr0_set | !exc.fpu_on * CR0_TS; }
        template <typename T> inline auto set_cr4() const { return T::fix_cr4_set; }
        template <typename T> inline auto msk_cr0() const { return T::fix_cr0_clr | set_cr0<T>(); }
        template <typename T> inline auto msk_cr4() const { return T::fix_cr4_clr | set_cr4<T>(); }

        inline auto set_exc() const { return BIT (EXC_AC) | !exc.fpu_on * BIT (EXC_NM); }

    public:
        union {
            Vmcb * const        vmcb;
            Vmcs * const        vmcs;
        };

        Exc_regs                exc;
        Refptr<Space_obj> const obj;
        Refptr<Space_hst> const hst;
        Refptr<Space_gst>       gst     { nullptr };
        Refptr<Space_pio>       pio     { nullptr };
        Refptr<Space_msr>       msr     { nullptr };
        Hazard                  hazard  { 0 };

        Cpu_regs (Refptr<Space_obj> &o, Refptr<Space_hst> &h, Refptr<Space_pio> &p) : vmcb { nullptr }, obj { std::move (o) }, hst { std::move (h) }, pio { std::move (p) } {}
        Cpu_regs (Refptr<Space_obj> &o, Refptr<Space_hst> &h, Vmcb *v) : vmcb { v }, obj { std::move (o) }, hst { std::move (h) }, hazard (Hazard::ILLEGAL) {}
        Cpu_regs (Refptr<Space_obj> &o, Refptr<Space_hst> &h, Vmcs *v) : vmcs { v }, obj { std::move (o) }, hst { std::move (h) }, hazard (Hazard::ILLEGAL) {}

        Space_obj *get_obj() const { return obj; }
        Space_hst *get_hst() const { return hst; }
        Space_gst *get_gst() const { return gst; }
        Space_pio *get_pio() const { return pio; }
        Space_msr *get_msr() const { return msr; }

        void fpu_ctrl (bool);
        void svm_set_cpu_pri (uint32_t) const;
        void svm_set_cpu_sec (uint32_t) const;
        void vmx_set_cpu_pri (uint32_t) const;
        void vmx_set_cpu_sec (uint32_t) const;
        void vmx_set_cpu_ter (uint64_t) const;

        inline void svm_set_bmp_exc() const { vmcb->intercept_exc = set_exc() | exc.intcpt_exc; }

        inline auto vmx_get_gst_cr0() const
        {
            auto msk = msk_cr0<Vmcs>();
            return (Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_CR0) & ~msk) | (exc.shadow_cr0 & msk);
        }

        inline auto vmx_get_gst_cr4() const
        {
            auto msk = msk_cr4<Vmcs>();
            return (Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_CR4) & ~msk) | (exc.shadow_cr4 & msk);
        }

        inline void vmx_set_gst_cr0 (uintptr_t v)
        {
            Vmcs::write (Vmcs::Encoding::CR0_READ_SHADOW, exc.shadow_cr0 = v);
            Vmcs::write (Vmcs::Encoding::GUEST_CR0, (v & ~msk_cr0<Vmcs>()) | set_cr0<Vmcs>());
        }

        inline void vmx_set_gst_cr4 (uintptr_t v)
        {
            Vmcs::write (Vmcs::Encoding::CR4_READ_SHADOW, exc.shadow_cr4 = v);
            Vmcs::write (Vmcs::Encoding::GUEST_CR4, (v & ~msk_cr4<Vmcs>()) | set_cr4<Vmcs>());
        }

        inline void vmx_set_msk_cr0() const { Vmcs::write (Vmcs::Encoding::CR0_MASK, msk_cr0<Vmcs>() | exc.intcpt_cr0); }
        inline void vmx_set_msk_cr4() const { Vmcs::write (Vmcs::Encoding::CR4_MASK, msk_cr4<Vmcs>() | exc.intcpt_cr4); }
        inline void vmx_set_bmp_exc() const { Vmcs::write (Vmcs::Encoding::BITMAP_EXC, set_exc()     | exc.intcpt_exc); }
};
