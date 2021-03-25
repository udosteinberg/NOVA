/*
 * Model-Specific Registers (MSR)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "compiler.hpp"
#include "std.hpp"
#include "types.hpp"

class Msr final
{
    public:
        // MSRs starting with IA32_ are architectural
        enum class Reg64 : unsigned
        {
            IA32_TSC                        = 0x10,
            IA32_PLATFORM_ID                = 0x17,
            IA32_APIC_BASE                  = 0x1b,
            IA32_FEATURE_CONTROL            = 0x3a,
            IA32_TSC_ADJUST                 = 0x3b,
            IA32_SPEC_CTRL                  = 0x48,         // IBRS or STIBP or SSBD or PSFD or IPRED_CTRL or RRSBA_CTRL or DDPD_U or BHI_CTRL
            IA32_PRED_CMD                   = 0x49,         // IBRS
            IA32_BIOS_SIGN_ID               = 0x8b,
            IA32_XAPIC_DISABLE_STATUS       = 0xbd,         // ARCH_CAPABILITIES
            FSB_FREQ                        = 0xcd,
            PLATFORM_INFO                   = 0xce,
            IA32_CORE_CAPABILITIES          = 0xcf,         // CORE_CAPABILITIES
            IA32_MPERF                      = 0xe7,
            IA32_APERF                      = 0xe8,
            IA32_MTRR_CAP                   = 0xfe,         // MTRR
            IA32_ARCH_CAPABILITIES          = 0x10a,        // ARCH_CAPABILITIES
            IA32_FLUSH_CMD                  = 0x10b,        // L1D_FLUSH
            IA32_TSX_FORCE_ABORT            = 0x10f,        // RTM_FORCE_ABORT
            IA32_TSX_CTRL                   = 0x122,        // ARCH_CAPABILITIES
            IA32_MCU_OPT_CTRL               = 0x123,        // SRBDS_CTRL
            IA32_SYSENTER_CS                = 0x174,        // SEP
            IA32_SYSENTER_ESP               = 0x175,        // SEP
            IA32_SYSENTER_EIP               = 0x176,        // SEP
            IA32_MCG_CAP                    = 0x179,
            IA32_MCG_STATUS                 = 0x17a,
            IA32_MCG_CTL                    = 0x17b,
            IA32_OVERCLOCKING_STATUS        = 0x195,
            IA32_PERF_STATUS                = 0x198,
            IA32_THERM_INTERRUPT            = 0x19b,
            IA32_THERM_STATUS               = 0x19c,
            IA32_PACKAGE_THERM_STATUS       = 0x1b1,
            IA32_DEBUGCTL                   = 0x1d9,
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
            IA32_TSC_DEADLINE               = 0x6e0,        // TSC_DEADLINE
            IA32_XSS                        = 0xda0,        // XSAVE
            IA32_UARCH_MISC_CTL             = 0x1b01,       // ARCH_CAPABILITIES

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

        enum class Arr64 : unsigned
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
            IA32_X2APIC                     = 0x800,        // X2APIC
        };

        static auto read (Reg64 msr)
        {
            uint32_t hi, lo;
            asm volatile ("rdmsr" : "=d" (hi), "=a" (lo) : "c" (msr));
            return static_cast<uint64_t>(hi) << 32 | lo;
        }

        static void write (Reg64 msr, uint64_t val)
        {
            asm volatile ("wrmsr" : : "d" (static_cast<uint32_t>(val >> 32)), "a" (static_cast<uint32_t>(val)), "c" (msr));
        }

        static auto read  (Arr64 r, unsigned b, unsigned n)       { return read (Reg64 { std::to_underlying (r) + b * n }); }
        static void write (Arr64 r, unsigned b, unsigned n, uint64_t v) { write (Reg64 { std::to_underlying (r) + b * n }, v); }
};
