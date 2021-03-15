/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
    uintptr_t   rax {};
    uintptr_t   rcx {};
    uintptr_t   rdx {};
    uintptr_t   rbx {};
    uintptr_t   rbp {};
    uintptr_t   rsi {};
    uintptr_t   rdi {};
    uintptr_t   r8  {};
    uintptr_t   r9  {};
    uintptr_t   r10 {};
    uintptr_t   r11 {};
    uintptr_t   r12 {};
    uintptr_t   r13 {};
    uintptr_t   r14 {};
    uintptr_t   r15 {};
};

static_assert (__is_standard_layout (Sys_regs) && sizeof (Sys_regs) == __SIZEOF_POINTER__ * 15);

struct Exc_regs
{
    Sys_regs            sys;

    union {
        struct {
            uintptr_t   err {};
            uintptr_t   vec {};
            uintptr_t   rip {};
            uintptr_t   cs  { SEL_USER_CODE };
            uintptr_t   rfl { RFL_AC | RFL_IF | RFL_1 };
            uintptr_t   rsp {};
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

    auto &ip() { return sys.rcx; }
    auto &sp() { return sys.r11; }

    // CPL0/1/2 (supervisor) CPL3 (user)
    bool user() const { return (cs & 3) == 3; }

    auto ep() const { return vec; }

    void set_ep (uintptr_t val) { vec = val; }
};

static_assert (__is_standard_layout (Exc_regs) && sizeof (Exc_regs) == __SIZEOF_POINTER__ * 22);

class alignas (16) Cpu_regs final
{
    private:
        template<typename T> uintptr_t set_cr0() const { return T::fix_cr0_set | !exc.fpu_on * CR0_TS; }
        template<typename T> uintptr_t set_cr4() const { return T::fix_cr4_set; }
        template<typename T> uintptr_t msk_cr0() const { return T::fix_cr0_clr | set_cr0<T>(); }
        template<typename T> uintptr_t msk_cr4() const { return T::fix_cr4_clr | set_cr4<T>(); }

        auto set_exc() const { return BIT (EXC_AC) | !exc.fpu_on * BIT (EXC_NM); }

    public:
        Exc_regs                exc;
        uintptr_t               cr2 {};
        union {
            Vmcb * const        vmcb;
            Vmcs * const        vmcs;
        };
        Refptr<Space_obj> const obj;
        Refptr<Space_hst> const hst;
        Refptr<Space_gst>       gst     { nullptr };
        Refptr<Space_pio>       pio     { nullptr };
        Refptr<Space_msr>       msr     { nullptr };
        Hazard                  hazard  { 0 };

        Cpu_regs (Refptr<Space_obj> &o, Refptr<Space_hst> &h, Refptr<Space_pio> &p) : vmcb { nullptr }, obj { std::move (o) }, hst { std::move (h) }, pio { std::move (p) } {}
        Cpu_regs (Refptr<Space_obj> &o, Refptr<Space_hst> &h, Vmcb *v) : vmcb { v }, obj { std::move (o) }, hst { std::move (h) }, hazard { Hazard::ILLEGAL } {}
        Cpu_regs (Refptr<Space_obj> &o, Refptr<Space_hst> &h, Vmcs *v) : vmcs { v }, obj { std::move (o) }, hst { std::move (h) }, hazard { Hazard::ILLEGAL } {}

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

        void svm_set_bmp_exc() const { vmcb->intercept_exc = set_exc() | exc.intcpt_exc; }
        void vmx_set_bmp_exc() const { Vmcs::write<Vmcs::Encoding::BITMAP_EXC>(set_exc() | exc.intcpt_exc); }

        void vmx_set_msk_cr0() const { Vmcs::write<Vmcs::Encoding::CR0_MASK>(msk_cr0<Vmcs>() | exc.intcpt_cr0); }
        void vmx_set_msk_cr4() const { Vmcs::write<Vmcs::Encoding::CR4_MASK>(msk_cr4<Vmcs>() | exc.intcpt_cr4); }

        void vmx_set_rsh_cr0 (uintptr_t v) { Vmcs::write<Vmcs::Encoding::CR0_READ_SHADOW>(exc.shadow_cr0 = v); }
        void vmx_set_rsh_cr4 (uintptr_t v) { Vmcs::write<Vmcs::Encoding::CR4_READ_SHADOW>(exc.shadow_cr4 = v); }

        void vmx_set_gst_cr0 (uintptr_t v)
        {
            vmx_set_rsh_cr0 (v);
            Vmcs::write<Vmcs::Encoding::GUEST_CR0>((v & ~msk_cr0<Vmcs>()) | set_cr0<Vmcs>());
        }

        void vmx_set_gst_cr4 (uintptr_t v)
        {
            vmx_set_rsh_cr4 (v);
            Vmcs::write<Vmcs::Encoding::GUEST_CR4>((v & ~msk_cr4<Vmcs>()) | set_cr4<Vmcs>());
        }

        auto vmx_get_gst_cr0() const
        {
            auto const msk { msk_cr0<Vmcs>() };
            return (Vmcs::read<Vmcs::Encoding::GUEST_CR0>() & ~msk) | (exc.shadow_cr0 & msk);
        }

        auto vmx_get_gst_cr4() const
        {
            auto const msk { msk_cr4<Vmcs>() };
            return (Vmcs::read<Vmcs::Encoding::GUEST_CR4>() & ~msk) | (exc.shadow_cr4 & msk);
        }
};
