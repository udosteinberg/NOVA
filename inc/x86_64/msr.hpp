/*
 * Model-Specific Registers (MSR)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "std.hpp"
#include "types.hpp"

class Msr final
{
    public:
        // MSRs starting with IA32_ are architectural
        enum class Register : unsigned
        {
            IA32_TSC                        = 0x10,
            IA32_PLATFORM_ID                = 0x17,
            IA32_APIC_BASE                  = 0x1b,
            IA32_FEATURE_CONTROL            = 0x3a,
            IA32_BIOS_SIGN_ID               = 0x8b,
            FSB_FREQ                        = 0xcd,
            PLATFORM_INFO                   = 0xce,
            IA32_MTRR_CAP                   = 0xfe,         // MTRR
            IA32_SYSENTER_CS                = 0x174,        // SEP
            IA32_SYSENTER_ESP               = 0x175,        // SEP
            IA32_SYSENTER_EIP               = 0x176,        // SEP
            IA32_MCG_CAP                    = 0x179,
            IA32_MCG_STATUS                 = 0x17a,
            IA32_MCG_CTL                    = 0x17b,
            IA32_PERF_CTL                   = 0x199,
            IA32_THERM_INTERRUPT            = 0x19b,
            IA32_MISC_ENABLE                = 0x1a0,
            TURBO_RATIO_LIMIT               = 0x1ad,
            IA32_PAT                        = 0x277,        // PAT
            IA32_MTRR_DEF_TYPE              = 0x2ff,        // MTRR
            IA32_VMX_BASIC                  = 0x480,        // VMX
            IA32_VMX_CTRL_PIN               = 0x481,        // VMX
            IA32_VMX_CTRL_CPU_PRI           = 0x482,        // VMX
            IA32_VMX_CTRL_EXI_PRI           = 0x483,        // VMX
            IA32_VMX_CTRL_ENT               = 0x484,        // VMX
            IA32_VMX_CTRL_MISC              = 0x485,        // VMX
            IA32_VMX_CR0_FIXED0             = 0x486,        // VMX
            IA32_VMX_CR0_FIXED1             = 0x487,        // VMX
            IA32_VMX_CR4_FIXED0             = 0x488,        // VMX
            IA32_VMX_CR4_FIXED1             = 0x489,        // VMX
            IA32_VMX_VMCS_ENUM              = 0x48a,        // VMX
            IA32_VMX_CTRL_CPU_SEC           = 0x48b,        // VMX
            IA32_VMX_EPT_VPID               = 0x48c,        // VMX
            IA32_VMX_TRUE_PIN               = 0x48d,        // VMX
            IA32_VMX_TRUE_CPU               = 0x48e,        // VMX
            IA32_VMX_TRUE_EXI               = 0x48f,        // VMX
            IA32_VMX_TRUE_ENT               = 0x490,        // VMX
            IA32_VMX_VMFUNC                 = 0x491,        // VMX
            IA32_VMX_CTRL_CPU_TER           = 0x492,        // VMX
            IA32_VMX_CTRL_EXI_SEC           = 0x493,        // VMX
            IA32_U_CET                      = 0x6a0,        // CET_SS or CET_IBT
            IA32_S_CET                      = 0x6a2,        // CET_SS or CET_IBT
            IA32_PL0_SSP                    = 0x6a4,        // CET_SS
            IA32_PL1_SSP                    = 0x6a5,        // CET_SS
            IA32_PL2_SSP                    = 0x6a6,        // CET_SS
            IA32_PL3_SSP                    = 0x6a7,        // CET_SS
            IA32_INTERRUPT_SSP_TABLE_ADDR   = 0x6a8,        // CET_SS
            IA32_TSC_DEADLINE               = 0x6e0,        // TSC_DEADLINE
            IA32_PM_ENABLE                  = 0x770,        // HWP
            IA32_HWP_CAPABILITIES           = 0x771,        // HWP
            IA32_HWP_REQUEST_PKG            = 0x772,        // HWP_PLR
            IA32_HWP_INTERRUPT              = 0x773,        // HWP_NTF
            IA32_HWP_REQUEST                = 0x774,        // HWP
            IA32_HWP_PECI_REQUEST_INFO      = 0x775,
            IA32_HWP_STATUS                 = 0x777,        // HWP
            IA32_L3_QOS_CFG                 = 0xc81,        // RDT-A and L3 CDP
            IA32_L2_QOS_CFG                 = 0xc82,        // RDT-A and L2 CDP
            IA32_PQR_ASSOC                  = 0xc8f,        // RDT-M or RDT-A
            IA32_XSS                        = 0xda0,        // XSAVE

            IA32_EFER                       = 0xc0000080,   // LM
            IA32_STAR                       = 0xc0000081,   // LM
            IA32_LSTAR                      = 0xc0000082,   // LM
            IA32_FMASK                      = 0xc0000084,   // LM
            IA32_FS_BASE                    = 0xc0000100,   // LM
            IA32_GS_BASE                    = 0xc0000101,   // LM
            IA32_KERNEL_GS_BASE             = 0xc0000102,   // LM
            IA32_TSC_AUX                    = 0xc0000103,   // RDPID or RDTSCP
            AMD_IPMR                        = 0xc0010055,
            AMD_SVM_HSAVE_PA                = 0xc0010117,
        };

        enum class Array : unsigned
        {
            IA32_MTRR_PHYS_BASE             = 0x200,        // IA32_MTRR_CAP[7:0] > 0
            IA32_MTRR_PHYS_MASK             = 0x201,        // IA32_MTRR_CAP[7:0] > 0
            IA32_MTRR_FIX64K_BASE           = 0x250,        // MTRR
            IA32_MTRR_FIX16K_BASE           = 0x258,        // MTRR
            IA32_MTRR_FIX4K_BASE            = 0x268,        // MTRR
            IA32_MC_CTL                     = 0x400,        // IA32_MCG_CAP[7:0] > 0
            IA32_MC_STATUS                  = 0x401,        // IA32_MCG_CAP[7:0] > 0
            IA32_MC_ADDR                    = 0x402,        // IA32_MCG_CAP[7:0] > 0
            IA32_MC_MISC                    = 0x403,        // IA32_MCG_CAP[7:0] > 0
            IA32_L3_MASK                    = 0xc90,        // RDT-A (max 128)
            IA32_L2_MASK                    = 0xd10,        // RDT-A (max 64)
            IA32_MB_THRT                    = 0xd50,        // RDT-A (max 64)
        };

        ALWAYS_INLINE
        static inline auto read (Register msr)
        {
            uint32 hi, lo;
            asm volatile ("rdmsr" : "=d" (hi), "=a" (lo) : "c" (msr));
            return static_cast<uint64>(hi) << 32 | lo;
        }

        ALWAYS_INLINE
        static inline void write (Register msr, uint64 val)
        {
            asm volatile ("wrmsr" : : "d" (static_cast<uint32>(val >> 32)), "a" (static_cast<uint32>(val)), "c" (msr));
        }

        static inline auto read  (Array r, unsigned b, unsigned n)     { return read (static_cast<Register>(std::to_underlying (r) + b * n)); }
        static inline void write (Array r, unsigned b, unsigned n, uint64 v) { write (static_cast<Register>(std::to_underlying (r) + b * n), v); }
};
