/*
 * Central Processing Unit (CPU)
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
#include "config.hpp"
#include "macros.hpp"
#include "types.hpp"

class Cpu
{
    private:
        ALWAYS_INLINE
        static inline void check_features (uint32 (&)[12]);

        ALWAYS_INLINE
        static inline void setup_msr();

        // Must agree with enum class Vendor below
        static constexpr char const *vendor_string[] =
        {
            "Unknown",
            "GenuineIntel",
            "AuthenticAMD"
        };

    public:
        enum class Vendor
        {
            UNKNOWN,
            INTEL,
            AMD,
        };

        enum class Feature
        {
            // 0x1.EDX
            MCE             = 0 * 32 +  7,      // Machine Check Exception
            SEP             = 0 * 32 + 11,      // SYSENTER/SYSEXIT Instructions
            MCA             = 0 * 32 + 14,      // Machine Check Architecture
            ACPI            = 0 * 32 + 22,      // Thermal Monitor and Software Controlled Clock Facilities
            // 0x1.ECX
            VMX             = 1 * 32 +  5,      // Virtual Machine Extensions
            PCID            = 1 * 32 + 17,      // Process Context Identifiers
            TSC_DEADLINE    = 1 * 32 + 24,      // TSC Deadline Support
            // 0x6.EAX
            ARAT            = 2 * 32 +  2,      // Always Running APIC Timer
            // 0x7.EBX
            SMEP            = 3 * 32 +  7,      // Supervisor Mode Execution Prevention
            SMAP            = 3 * 32 + 20,      // Supervisor Mode Access Prevention
            // 0x7.ECX
            UMIP            = 4 * 32 +  2,      // User Mode Instruction Prevention
            // 0x7.EDX
            HYBRID          = 5 * 32 + 15,      // Hybrid Processor
            // 0x80000001.EDX
            GB_PAGES        = 6 * 32 + 26,      // 1GB-Pages Support
            LM              = 6 * 32 + 29,      // Long Mode Support
            // 0x80000001.ECX
            CMP_LEGACY      = 7 * 32 +  1,
            SVM             = 7 * 32 +  2,
        };

        static inline unsigned  boot_lock   asm ("boot_lock");
        static inline unsigned  online;
        static inline uint8     acpi_id[NUM_CPU];
        static inline uint8     apic_id[NUM_CPU];

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
        static unsigned patch               CPULOCAL;

        static uint32 features[8]           CPULOCAL;
        static bool bsp                     CPULOCAL;

        static void init();

        ALWAYS_INLINE
        static inline bool feature (Feature f)
        {
            return features[static_cast<unsigned>(f) / 32] & BIT (static_cast<unsigned>(f) % 32);
        }

        ALWAYS_INLINE
        static inline void defeature (Feature f)
        {
            features[static_cast<unsigned>(f) / 32] &= ~BIT (static_cast<unsigned>(f) % 32);
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

        static inline void halt()
        {
            asm volatile ("sti; hlt; cli" : : : "memory");
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
