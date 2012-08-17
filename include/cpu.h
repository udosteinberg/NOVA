/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "compiler.h"
#include "types.h"

class Cpu
{
    private:
        static char const * const vendor_string[];

        ALWAYS_INLINE
        static inline void check_features();

        ALWAYS_INLINE
        static inline void setup_thermal();

        ALWAYS_INLINE
        static inline void setup_sysenter();

    public:
        enum Vendor
        {
            UNKNOWN,
            INTEL,
            AMD
        };

        enum Feature
        {
            FEAT_MCE            =  7,
            FEAT_SEP            = 11,
            FEAT_MCA            = 14,
            FEAT_ACPI           = 22,
            FEAT_HTT            = 28,
            FEAT_VMX            = 37,
            FEAT_SMEP           = 103,
            FEAT_CMP_LEGACY     = 161,
            FEAT_SVM            = 162,
        };

        enum
        {
            EXC_DB          = 1,
            EXC_NM          = 7,
            EXC_TS          = 10,
            EXC_GP          = 13,
            EXC_PF          = 14,
            EXC_AC          = 17,
            EXC_MC          = 18,
        };

        enum
        {
            CR0_PE          = 1ul << 0,         // 0x1
            CR0_MP          = 1ul << 1,         // 0x2
            CR0_EM          = 1ul << 2,         // 0x4
            CR0_TS          = 1ul << 3,         // 0x8
            CR0_ET          = 1ul << 4,         // 0x10
            CR0_NE          = 1ul << 5,         // 0x20
            CR0_WP          = 1ul << 16,        // 0x10000
            CR0_AM          = 1ul << 18,        // 0x40000
            CR0_NW          = 1ul << 29,        // 0x20000000
            CR0_CD          = 1ul << 30,        // 0x40000000
            CR0_PG          = 1ul << 31         // 0x80000000
        };

        enum
        {
            CR4_DE          = 1UL << 3,         // 0x8
            CR4_PSE         = 1UL << 4,         // 0x10
            CR4_PAE         = 1UL << 5,         // 0x20
            CR4_MCE         = 1UL << 6,         // 0x40
            CR4_PGE         = 1UL << 7,         // 0x80
            CR4_OSFXSR      = 1UL << 9,         // 0x200
            CR4_OSXMMEXCPT  = 1UL << 10,        // 0x400
            CR4_VMXE        = 1UL << 13,        // 0x2000
            CR4_SMXE        = 1UL << 14,        // 0x4000
            CR4_SMEP        = 1UL << 20,        // 0x100000
        };

        enum
        {
            EFER_SVME       = 1ul << 12,        // 0x1000
        };

        enum
        {
            EFL_CF      = 1ul << 0,             // 0x1
            EFL_PF      = 1ul << 2,             // 0x4
            EFL_AF      = 1ul << 4,             // 0x10
            EFL_ZF      = 1ul << 6,             // 0x40
            EFL_SF      = 1ul << 7,             // 0x80
            EFL_TF      = 1ul << 8,             // 0x100
            EFL_IF      = 1ul << 9,             // 0x200
            EFL_DF      = 1ul << 10,            // 0x400
            EFL_OF      = 1ul << 11,            // 0x800
            EFL_IOPL    = 3ul << 12,            // 0x3000
            EFL_NT      = 1ul << 14,            // 0x4000
            EFL_RF      = 1ul << 16,            // 0x10000
            EFL_VM      = 1ul << 17,            // 0x20000
            EFL_AC      = 1ul << 18,            // 0x40000
            EFL_VIF     = 1ul << 19,            // 0x80000
            EFL_VIP     = 1ul << 20,            // 0x100000
            EFL_ID      = 1ul << 21             // 0x200000
        };

        static mword    boot_lock           asm ("boot_lock");

        static unsigned online;
        static unsigned id                  CPULOCAL_HOT;
        static unsigned hazard              CPULOCAL_HOT;
        static unsigned package             CPULOCAL;
        static unsigned core                CPULOCAL;
        static unsigned thread              CPULOCAL;

        static Vendor   vendor              CPULOCAL;
        static unsigned platform            CPULOCAL;
        static unsigned family              CPULOCAL;
        static unsigned model               CPULOCAL;
        static unsigned stepping            CPULOCAL;
        static unsigned brand               CPULOCAL;
        static unsigned patch               CPULOCAL;
        static unsigned row                 CPULOCAL;

        static uint32 name[12]              CPULOCAL;
        static uint32 features[6]           CPULOCAL;
        static bool bsp                     CPULOCAL;

        static void init();

        ALWAYS_INLINE
        static inline bool feature (Feature feat)
        {
            return features[feat / 32] & 1u << feat % 32;
        }

        ALWAYS_INLINE
        static inline void preempt_disable()
        {
            asm volatile ("cli" : : : "memory");
        }

        ALWAYS_INLINE
        static inline void preempt_enable()
        {
            asm volatile ("sti" : : : "memory");
        }

        ALWAYS_INLINE
        static inline void cpuid (unsigned leaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf));
        }

        ALWAYS_INLINE
        static inline void cpuid (unsigned leaf, unsigned subleaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf), "c" (subleaf));
        }
};
