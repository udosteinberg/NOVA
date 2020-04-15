/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "msr.hpp"
#include "selectors.hpp"
#include "spinlock.hpp"
#include "types.hpp"

class Cpu final
{
    private:
        static constexpr apic_t invalid_topology { BIT_RANGE (31, 0) };

        // Must agree with enum class Vendor below
        static constexpr char const *vendor_string[] { "Unknown", "GenuineIntel", "AuthenticAMD" };

        enum class Cstate : unsigned
        {
            C0  =  0,
            C1  =  8,
            C3  = 16,
            C6  = 24,
            C7  = 32,
            C8  = 40,
            C9  = 48,
            C10 = 56,
        };

        static uint32_t cstates CPULOCAL;
        static uint64_t csthint CPULOCAL;

        static auto supports (Cstate c) { return cstates >> std::to_underlying (c) / 2 & BIT_RANGE (3, 0); }

        static constexpr struct scaleable_bus { uint8_t m, d; }
            freq_atom[BIT (4)] { { 5, 6 }, { 1, 1 }, { 4, 3 }, { 7, 6 }, { 4, 5 }, { 14, 15 }, { 9, 10 }, { 8, 9 }, { 7, 8 } },
            freq_core[BIT (3)] { { 8, 3 }, { 4, 3 }, { 2, 1 }, { 5, 3 }, { 10, 3 }, { 1, 1 }, { 4, 1 } };

        static void enumerate_clocks (uint32_t &, uint32_t &, scaleable_bus const *, unsigned);
        static void enumerate_clocks (uint32_t &, uint32_t &);

        static void enumerate_topology (uint32_t, uint32_t &, uint32_t (&)[4]);
        static void enumerate_features (uint32_t &, uint32_t &, uint32_t (&)[4], uint32_t (&)[12]);

        static void setup_msr();
        static void setup_cst();
        static void setup_pst();

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
            // EAX=0x1 (ECX)
            MONITOR                 =  0 * 32 +  3,     // MONITOR/MWAIT Support
            VMX                     =  0 * 32 +  5,     // Virtual Machine Extensions
            EIST                    =  0 * 32 +  7,     // Enhanced Intel SpeedStep Technology
            PCID                    =  0 * 32 + 17,     // Process Context Identifiers
            X2APIC                  =  0 * 32 + 21,     // x2APIC Support
            TSC_DEADLINE            =  0 * 32 + 24,     // TSC Deadline Support
            // EAX=0x1 (EDX)
            MCE                     =  1 * 32 +  7,     // Machine Check Exception
            SEP                     =  1 * 32 + 11,     // SYSENTER/SYSEXIT Instructions
            MCA                     =  1 * 32 + 14,     // Machine Check Architecture
            PAT                     =  1 * 32 + 16,     // Page Attribute Table
            ACPI                    =  1 * 32 + 22,     // Thermal Monitor and Software Controlled Clock Facilities
            HTT                     =  1 * 32 + 28,     // Hyper-Threading Technology
            // EAX=0x6 (EAX)
            TURBO_BOOST             =  2 * 32 +  1,     // Turbo Boost Technology
            ARAT                    =  2 * 32 +  2,     // Always Running APIC Timer
            HWP                     =  2 * 32 +  7,     // HWP Baseline Resource and Capability
            HWP_NTF                 =  2 * 32 +  8,     // HWP Notification
            HWP_ACT                 =  2 * 32 +  9,     // HWP Activity Window
            HWP_EPP                 =  2 * 32 + 10,     // HWP Energy Performance Preference
            HWP_PLR                 =  2 * 32 + 11,     // HWP Package Level Request
            HWP_CAP                 =  2 * 32 + 15,     // HWP Capabilities
            HWP_PECI                =  2 * 32 + 16,     // HWP PECI Override
            HWP_FLEX                =  2 * 32 + 17,     // HWP Flexible
            HWP_FAM                 =  2 * 32 + 18,     // HWP Fast Access Mode
            // EAX=0x7 ECX=0x0 (EBX)
            SMEP                    =  3 * 32 +  7,     // Supervisor Mode Execution Prevention
            SMAP                    =  3 * 32 + 20,     // Supervisor Mode Access Prevention
            // EAX=0x7 ECX=0x0 (ECX)
            UMIP                    =  4 * 32 +  2,     // User Mode Instruction Prevention
            // EAX=0x7 ECX=0x0 (EDX)
            SRBDS_CTRL              =  5 * 32 +  9,     // Special Register Buffer Data Sampling
            MD_CLEAR                =  5 * 32 + 10,     // Microarchitectural Data Clear
            HYBRID                  =  5 * 32 + 15,     // Hybrid Processor
            IBRS                    =  5 * 32 + 26,     // Indirect Branch Restricted Speculation
            STIBP                   =  5 * 32 + 27,     // Single Thread Indirect Branch Predictors
            L1D_FLUSH               =  5 * 32 + 28,     // L1 Data Cache Flushing
            ARCH_CAPABILITIES       =  5 * 32 + 29,     // Arch Capabilities
            CORE_CAPABILITIES       =  5 * 32 + 30,     // Core Capabilities
            SSBD                    =  5 * 32 + 31,     // Speculative Store Bypass Disable
            // EAX=0x7 ECX=0x1 (EAX)
            // EAX=0x7 ECX=0x1 (EBX)
            // EAX=0x7 ECX=0x1 (ECX)
            // EAX=0x7 ECX=0x1 (EDX)
            APX_F                   =  9 * 32 + 21,     // APX Foundation
            // EAX=0x7 ECX=0x2 (EDX)
            PSFD                    = 10 * 32 +  0,
            IPRED_CTRL              = 10 * 32 +  1,
            RRSBA_CTRL              = 10 * 32 +  2,
            DDPD_U                  = 10 * 32 +  3,
            BHI_CTRL                = 10 * 32 +  4,
            MCDT_NO                 = 10 * 32 +  5,
            // EAX=0x80000001 (ECX)
            SVM                     = 11 * 32 +  2,
            // EAX=0x80000001 (EDX)
            GB_PAGES                = 12 * 32 + 26,     // 1GB-Pages Support
            LM                      = 12 * 32 + 29,     // Long Mode Support
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
            .star  = static_cast<uint64_t>(SEL_KERN_DATA) << 48 | static_cast<uint64_t>(SEL_KERN_CODE) << 32,
            .lstar = reinterpret_cast<uintptr_t>(&entry_sys),
            .fmask = RFL_VIP | RFL_VIF | RFL_AC | RFL_VM | RFL_RF | RFL_NT | RFL_IOPL | RFL_DF | RFL_IF | RFL_TF,
            .kernel_gs_base = 0,
        };

        static cpu_t        id              CPULOCAL_HOT;
        static apic_t       topology        CPULOCAL_HOT;
        static unsigned     hazard          CPULOCAL_HOT;
        static Vendor       vendor          CPULOCAL;
        static unsigned     platform        CPULOCAL;
        static unsigned     family          CPULOCAL;
        static unsigned     model           CPULOCAL;
        static unsigned     stepping        CPULOCAL;
        static unsigned     patch           CPULOCAL;
        static uint32_t     features[13]    CPULOCAL;
        static bool         bsp             CPULOCAL;

        static inline cpu_t                 count  { 0 };
        static inline Atomic<cpu_t>         online { 0 };

        static void init();
        static void fini();
        static void halt();

        static bool feature (Feature f)
        {
            return features[std::to_underlying (f) / 32] & BIT (std::to_underlying (f) % 32);
        }

        static void defeature (Feature f)
        {
            features[std::to_underlying (f) / 32] &= ~BIT (std::to_underlying (f) % 32);
        }

        static void preemption_disable()    { asm volatile ("cli" : : : "memory"); }
        static void preemption_enable()     { asm volatile ("sti" : : : "memory"); }
        static void preemption_point()      { asm volatile ("sti; nop; cli" : : : "memory"); }

        static void cpuid (unsigned leaf, uint32_t &eax, uint32_t &ebx, uint32_t &ecx, uint32_t &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf));
        }

        static void cpuid (unsigned leaf, unsigned subleaf, uint32_t &eax, uint32_t &ebx, uint32_t &ecx, uint32_t &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf), "c" (subleaf));
        }

        static auto remote_topology (cpu_t c) { return *Kmem::loc_to_glob (c, &topology); }

        static auto find_by_topology (uint32_t t)
        {
            for (cpu_t c { 0 }; c < count; c++)
                if (remote_topology (c) == t)
                    return c;

            return static_cast<cpu_t>(-1);
        }
};
