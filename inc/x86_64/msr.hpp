/*
 * Model-Specific Registers (MSR)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "compiler.hpp"
#include "macros.hpp"
#include "types.hpp"

class Msr
{
    public:

        // MSRs starting with IA32_ are architectural
        enum Register
        {
            IA32_TSC                = 0x10,
            IA32_PLATFORM_ID        = 0x17,
            IA32_APIC_BASE          = 0x1b,
            IA32_FEATURE_CONTROL    = 0x3a,
            IA32_BIOS_SIGN_ID       = 0x8b,
            IA32_SMM_MONITOR_CTL    = 0x9b,
            PLATFORM_INFO           = 0xce,
            IA32_MTRR_CAP           = 0xfe,
            IA32_SYSENTER_CS        = 0x174,
            IA32_SYSENTER_ESP       = 0x175,
            IA32_SYSENTER_EIP       = 0x176,
            IA32_MCG_CAP            = 0x179,
            IA32_MCG_STATUS         = 0x17a,
            IA32_MCG_CTL            = 0x17b,
            IA32_PERF_CTL           = 0x199,
            IA32_THERM_INTERRUPT    = 0x19b,
            IA32_THERM_STATUS       = 0x19c,
            IA32_MISC_ENABLE        = 0x1a0,
            TURBO_RATIO_LIMIT       = 0x1ad,
            IA32_DEBUG_CTL          = 0x1d9,
            IA32_MTRR_PHYS_BASE     = 0x200,
            IA32_MTRR_PHYS_MASK     = 0x201,
            IA32_MTRR_FIX64K_BASE   = 0x250,
            IA32_MTRR_FIX16K_BASE   = 0x258,
            IA32_MTRR_FIX4K_BASE    = 0x268,
            IA32_PAT                = 0x277,
            IA32_MTRR_DEF_TYPE      = 0x2ff,

            IA32_MCI_CTL            = 0x400,
            IA32_MCI_STATUS         = 0x401,

            IA32_VMX_BASIC          = 0x480,
            IA32_VMX_CTRL_PIN       = 0x481,
            IA32_VMX_CTRL_PROC1     = 0x482,
            IA32_VMX_CTRL_EXIT      = 0x483,
            IA32_VMX_CTRL_ENTRY     = 0x484,
            IA32_VMX_CTRL_MISC      = 0x485,
            IA32_VMX_CR0_FIXED0     = 0x486,
            IA32_VMX_CR0_FIXED1     = 0x487,
            IA32_VMX_CR4_FIXED0     = 0x488,
            IA32_VMX_CR4_FIXED1     = 0x489,
            IA32_VMX_VMCS_ENUM      = 0x48a,
            IA32_VMX_CTRL_PROC2     = 0x48b,
            IA32_VMX_EPT_VPID       = 0x48c,
            IA32_VMX_TRUE_PIN       = 0x48d,
            IA32_VMX_TRUE_PROC1     = 0x48e,
            IA32_VMX_TRUE_EXIT      = 0x48f,
            IA32_VMX_TRUE_ENTRY     = 0x490,
            IA32_VMX_VMFUNC         = 0x491,
            IA32_VMX_CTRL_PROC3     = 0x492,

            IA32_DS_AREA            = 0x600,
            IA32_TSC_DEADLINE       = 0x6e0,
            IA32_X2APIC             = 0x800,
            IA32_EFER               = 0xc0000080,
            IA32_STAR               = 0xc0000081,
            IA32_LSTAR              = 0xc0000082,
            IA32_FMASK              = 0xc0000084,
            IA32_KERNEL_GS_BASE     = 0xc0000102,

            AMD_IPMR                = 0xc0010055,
            AMD_SVM_HSAVE_PA        = 0xc0010117,
        };

        enum Feature_Control
        {
            FEATURE_LOCKED          = BIT (0),
            FEATURE_VMX_I_SMX       = BIT (1),
            FEATURE_VMX_O_SMX       = BIT (2),
        };

        ALWAYS_INLINE
        static inline uint64 read (Register msr)
        {
            uint32 hi, lo;
            asm volatile ("rdmsr" : "=d" (hi), "=a" (lo) : "c" (msr));
            return static_cast<uint64>(hi) << 32 | lo;
        }

        ALWAYS_INLINE
        static inline void write (Register msr, uint64 val)
        {
            asm volatile ("wrmsr" : : "d" (static_cast<uint32> (val >> 32)), "a" (static_cast<uint32> (val)), "c" (msr));
        }

        struct State
        {
            uint64  star            { 0 };
            uint64  lstar           { 0 };
            uint64  fmask           { 0 };
            uint64  kernel_gs_base  { 0 };

            ALWAYS_INLINE
            static inline void make_current (State const &o, State const &n)
            {
                if (EXPECT_FALSE (o.star != n.star))
                    write (IA32_STAR, n.star);
                if (EXPECT_FALSE (o.lstar != n.lstar))
                    write (IA32_LSTAR, n.lstar);
                if (EXPECT_FALSE (o.fmask != n.fmask))
                    write (IA32_FMASK, n.fmask);
                if (EXPECT_FALSE (o.kernel_gs_base != n.kernel_gs_base))
                    write (IA32_KERNEL_GS_BASE, n.kernel_gs_base);
            }
        };
};
