/*
 * Central Processing Unit (CPU)
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

#include "arch.hpp"
#include "atomic.hpp"
#include "compiler.hpp"
#include "config.hpp"
#include "extern.hpp"
#include "kmem.hpp"
#include "macros.hpp"
#include "msr.hpp"
#include "selectors.hpp"
#include "spinlock.hpp"
#include "std.hpp"
#include "types.hpp"

class Cpu final
{
    private:
        // Must agree with enum class Vendor below
        static constexpr char const *vendor_string[] { "Unknown", "GenuineIntel", "AuthenticAMD" };

        static constexpr struct scaleable_bus { uint8 m, d; }
            freq_atom[BIT (4)] { { 5, 6 }, { 1, 1 }, { 4, 3 }, { 7, 6 }, { 4, 5 }, { 14, 15 }, { 9, 10 }, { 8, 9 }, { 7, 8 } },
            freq_core[BIT (3)] { { 8, 3 }, { 4, 3 }, { 2, 1 }, { 5, 3 }, { 10, 3 }, { 1, 1 }, { 4, 1 } };

        static inline void enumerate_clocks (uint32 &, uint32 &, scaleable_bus const *, unsigned);
        static inline void enumerate_clocks (uint32 &, uint32 &);

        ALWAYS_INLINE
        static inline void enumerate_features (uint32 &, uint32 &, uint32 (&)[12], unsigned &, unsigned &, unsigned &);

        ALWAYS_INLINE
        static inline void setup_msr();

        ALWAYS_INLINE
        static inline void setup_pstate();

        ALWAYS_INLINE
        static inline uint32 remote_topology (unsigned c) { return *Kmem::loc_to_glob (&topology, c); }

        static inline Spinlock boot_lock    asm ("__boot_lock");

    public:
        enum class Vendor : unsigned
        {
            UNKNOWN,
            INTEL,
            AMD,
        };

        enum class Feature : unsigned
        {
            // 0x1.EDX
            MCE             = 0 * 32 +  7,      // Machine Check Exception
            SEP             = 0 * 32 + 11,      // SYSENTER/SYSEXIT Instructions
            MCA             = 0 * 32 + 14,      // Machine Check Architecture
            PAT             = 0 * 32 + 16,      // Page Attribute Table
            ACPI            = 0 * 32 + 22,      // Thermal Monitor and Software Controlled Clock Facilities
            // 0x1.ECX
            VMX             = 1 * 32 +  5,      // Virtual Machine Extensions
            EIST            = 1 * 32 +  7,      // Enhanced Intel SpeedStep Technology
            PCID            = 1 * 32 + 17,      // Process Context Identifiers
            TSC_DEADLINE    = 1 * 32 + 24,      // TSC Deadline Support
            XSAVE           = 1 * 32 + 26,      // XCR0, XSETBV/XGETBV/XSAVE/XRSTOR Instructions
            // 0x6.EAX
            TURBO_BOOST     = 2 * 32 +  1,      // Turbo Boost Technology
            ARAT            = 2 * 32 +  2,      // Always Running APIC Timer
            HWP             = 2 * 32 +  7,      // HWP
            HWP_NTF         = 2 * 32 +  8,      // HWP Notification
            HWP_ACT         = 2 * 32 +  9,      // HWP Activity Window
            HWP_EPP         = 2 * 32 + 10,      // HWP Energy Performance Preference
            HWP_PLR         = 2 * 32 + 11,      // HWP Package Level Request
            HWP_CAP         = 2 * 32 + 15,      // HWP Capabilities
            HWP_PECI        = 2 * 32 + 16,      // HWP PECI Override
            HWP_FLEX        = 2 * 32 + 17,      // HWP Flexible
            HWP_FAM         = 2 * 32 + 18,      // HWP Fast Access Mode
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

        struct State_sys
        {
            uint64  star            { 0 };
            uint64  lstar           { 0 };
            uint64  fmask           { 0 };
            uint64  kernel_gs_base  { 0 };
        };

        static inline State_sys const hst_sys
        {
            .star  = static_cast<uint64>(SEL_USER_CODE32) << 48 | static_cast<uint64>(SEL_KERN_CODE) << 32,
            .lstar = reinterpret_cast<uintptr_t>(&entry_sys),
            .fmask = RFL_VIP | RFL_VIF | RFL_AC | RFL_VM | RFL_RF | RFL_NT | RFL_IOPL | RFL_DF | RFL_IF | RFL_TF,
            .kernel_gs_base = 0,
        };

        static unsigned     id              CPULOCAL_HOT;
        static unsigned     hazard          CPULOCAL_HOT;
        static Vendor       vendor          CPULOCAL;
        static unsigned     platform        CPULOCAL;
        static unsigned     family          CPULOCAL;
        static unsigned     model           CPULOCAL;
        static unsigned     stepping        CPULOCAL;
        static unsigned     patch           CPULOCAL;
        static uint32       features[8]     CPULOCAL;
        static uint32       topology        CPULOCAL;
        static bool         bsp             CPULOCAL;

        static inline Atomic<unsigned>      online { 0 };

        static inline unsigned  count;
        static inline uint8     acpi_id[NUM_CPU];
        static inline uint8     apic_id[NUM_CPU];

        static void init();
        static void fini();

        ALWAYS_INLINE
        static inline bool feature (Feature f)
        {
            return features[std::to_underlying (f) / 32] & BIT (std::to_underlying (f) % 32);
        }

        ALWAYS_INLINE
        static inline void defeature (Feature f)
        {
            features[std::to_underlying (f) / 32] &= ~BIT (std::to_underlying (f) % 32);
        }

        ALWAYS_INLINE
        static inline void preemption_disable() { asm volatile ("cli" : : : "memory"); }

        ALWAYS_INLINE
        static inline void preemption_enable() { asm volatile ("sti" : : : "memory"); }

        ALWAYS_INLINE
        static inline void preemption_point() { asm volatile ("sti; nop; cli" : : : "memory"); }

        ALWAYS_INLINE
        static inline void halt() { asm volatile ("sti; hlt; cli" : : : "memory"); }

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
        static inline unsigned find_by_topology (uint32 t)
        {
            for (unsigned c = 0; c < count; c++)
                if (remote_topology (c) == t)
                    return c;

            return -1U;
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
