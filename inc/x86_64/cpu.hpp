/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
#include "compiler.hpp"
#include "config.hpp"
#include "extern.hpp"
#include "msr.hpp"
#include "selectors.hpp"
#include "spinlock.hpp"
#include "types.hpp"

class Cpu
{
    private:
        static char const * const vendor_string[];

        ALWAYS_INLINE
        static inline void check_features();

        ALWAYS_INLINE
        static inline void setup_msr();

        ALWAYS_INLINE
        static inline void setup_pcid();

        static inline Spinlock boot_lock    asm ("__boot_lock");

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
            FEAT_PCID           = 49,
            FEAT_TSC_DEADLINE   = 56,
            FEAT_SMEP           = 103,
            FEAT_SMAP           = 116,
            FEAT_1GB_PAGES      = 154,
            FEAT_LM             = 157,
            FEAT_CMP_LEGACY     = 161,
            FEAT_SVM            = 162,
        };

        struct State_sys
        {
            uint64_t    star            { 0 };
            uint64_t    lstar           { 0 };
            uint64_t    fmask           { 0 };
            uint64_t    kernel_gs_base  { 0 };
        };

        static inline State_sys const hst_sys
        {
            .star  = static_cast<uint64_t>(SEL_USER_CODE32) << 48 | static_cast<uint64_t>(SEL_KERN_CODE) << 32,
            .lstar = reinterpret_cast<uintptr_t>(&entry_sys),
            .fmask = RFL_VIP | RFL_VIF | RFL_AC | RFL_VM | RFL_RF | RFL_NT | RFL_IOPL | RFL_DF | RFL_IF | RFL_TF,
            .kernel_gs_base = 0,
        };

        static unsigned online;
        static uint8    acpi_id[NUM_CPU];
        static uint8    apic_id[NUM_CPU];

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

        static uint32 name[12]              CPULOCAL;
        static uint32 features[6]           CPULOCAL;
        static bool bsp                     CPULOCAL;

        static void init();

        ALWAYS_INLINE
        static inline bool feature (Feature f)
        {
            return features[f / 32] & 1U << f % 32;
        }

        ALWAYS_INLINE
        static inline void defeature (Feature f)
        {
            features[f / 32] &= ~(1U << f % 32);
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

        ALWAYS_INLINE
        static unsigned find_by_apic_id (unsigned x)
        {
            for (unsigned i = 0; i < NUM_CPU; i++)
                if (apic_id[i] == x)
                    return i;

            return ~0U;
        }
};
