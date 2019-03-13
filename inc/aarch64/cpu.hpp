/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "memory.hpp"
#include "spinlock.hpp"
#include "types.hpp"

class Cpu
{
    private:
        static void enumerate_features();
        static void enumerate_topology();

    public:
        enum class Cpu_feature
        {
            EL0         =  0,   // EL0 Support AArch64 only (1), AArch64+AArch32 (2)
            EL1         =  1,   // EL1 Support AArch64 only (1), AArch64+AArch32 (2)
            EL2         =  2,   // EL2 Support AArch64 only (1), AArch64+AArch32 (2)
            EL3         =  3,   // EL3 Support AArch64 only (1), AArch64+AArch32 (2)
            FP          =  4,   // Floating Point (0), Half-Precision (1), Unsupported (15)
            ADVSIMD     =  5,   // Advanced SIMD (0), ARMv8.2-FP16 (1), Unsupported (15)
            GIC         =  6,   // GIC System Registers v3.0/4.0 (1), v4.1 (3)
            RAS         =  7,   // ARMv8.0-RAS (1), ARMv8.4-RAS (2)
            SVE         =  8,   // ARMv8.2-SVE (1)
            SEL2        =  9,   // ARMv8.4-SecEL2 (1)
            MPAM        = 10,   // ARMv8.2-MPAM (1)
            AMU         = 11,   // ARMv8.4-AMUv1 (1), ARMv8.6-AMUv1 (2)
            DIT         = 12,   // ARMv8.4-DIT (1)
            CSV2        = 14,   // ARMv8.0-CSV2 (1,2)
            CSV3        = 15,   // ARMv8.0-CSV3 (1)
            BT          = 16,   // ARMv8.5-BTI (1)
            SSBS        = 17,   // ARMv8.0-SSBS (1,2)
            MTE         = 18,   // ARMv8.5-MemTag (1,2)
            RAS_FRAC    = 19,   // ARMv8.4-RAS (1)
            MPAM_FRAC   = 20,   // ARMv8.6-MPAM (1)
        };

        enum class Dbg_feature
        {
            DEBUGVER    =  0,   // ARMv8.0-Debug (6), ARMv8.1-Debug (7), ARMv8.2-Debug (8), ARMv8.4-Debug (9)
            TRACEVER    =  1,   // Trace Support
            PMUVER      =  2,   // PMUv3 (1), ARMv8.1-PMU (4), ARMv8.4-PMU (5), ARMv8.5-PMU (6)
            BRPS        =  3,   // Number of Breakpoints - 1
            WRPS        =  5,   // Number of Watchpoints - 1
            CTXCMPS     =  7,   // Number of Context-Aware Breakpoints - 1
            PMSVER      =  8,   // SPE (1), ARMv8.3-SPE (2)
            DOUBLELOCK  =  9,   // ARMv8.0-DoubleLock (0)
            TRACEFILT   = 10,   // ARMv8.4-Trace (1)
            TRACEBUFFER = 11,   // ARMv8.x-TRBE (1)
            MTPMU       = 12,   // ARMv8.6-MTPMU (1)
        };

        enum class Isa_feature
        {
            AES         =  1,   // ARMv8.0-AES AESE/AESD/AESMC/AESIMC (1), +PMULL/PMULL2 (2)
            SHA1        =  2,   // ARMv8.0-SHA SHA1C/SHA1P/SHA1M/SHA1H/SHA1SU0/SHA1SU1 (1)
            SHA2        =  3,   // ARMv8.2-SHA SHA256H/SHA256H2/SHA256SU0/SHA256SU1 (1), +SHA512H/SHA512H2/SHA512SU0/SHA512SU1 (2)
            CRC32       =  4,   // CRC32B/CRC32H/CRC32W/CRC32X/CRC32CB/CRC32CH/CRC32CW/CRC32CX (1)
            ATOMIC      =  5,   // ARMv8.1-LSE (2)
            TME         =  6,   // ARMv8.x-TME (1)
            RDM         =  7,   // ARMv8.1-RDMA (1)
            SHA3        =  8,   // ARMv8.2-SHA (1)
            SM3         =  9,   // ARMv8.2-SM (1)
            SM4         = 10,   // ARMv8.2-SM (1)
            DP          = 11,   // ARMv8.2-DotProd (1)
            FHM         = 12,   // ARMv8.2-FHM (1)
            TS          = 13,   // ARMv8.4-CondM (1), ARMv8.5-CondM (2)
            TLB         = 14,   // ARMv8.4-TLBI (1,2)
            RNDR        = 15,   // ARMv8.5-RNG (1)
            DPB         = 16,   // ARMv8.2-DCPoP (1), ARMv8.2-DCCVADP (2)
            APA         = 17,   // ARMv8.3-PAuth (1)
            API         = 18,   // ARMv8.3-PAuth (1)
            JSCVT       = 19,   // ARMv8.3-JSConv (1)
            FCMA        = 20,   // ARMv8.3-CompNum (1)
            LRCPC       = 21,   // ARMv8.3-RCPC (1), ARMv8.4-RCPC (2)
            GPA         = 22,   // ARMv8.3-PAuth (1)
            GPI         = 23,   // ARMv8.3-PAuth (1)
            FRINTTS     = 24,   // ARMv8.5-FRINT (1)
            SB          = 25,   // ARMv8.0-SB (1)
            SPECRES     = 26,   // ARMv8.0-PredInv (1)
            BF16        = 27,   // ARMv8.2-BF16 (1)
            DGH         = 28,   // ARMv8.0-DGH (1)
            I8MM        = 29,   // ARMv8.2-I8MM (1)
        };

        enum class Mem_feature
        {
            PARANGE     =  0,   // ARMv8.2-LPA (6)
            ASIDBITS    =  1,   // Number of ASID Bits
            BIGEND      =  2,   // Mixed Endian Support
            SNSMEM      =  3,   // Secure/Non-Secure Memory Distinction
            BIGEND_EL0  =  4,   // Mixed Endian Support at EL0
            TGRAN16     =  5,   // Stage-1 Granule 16KB
            TGRAN64     =  6,   // Stage-1 Granule 64KB
            TGRAN4      =  7,   // Stage-1 Granule  4KB
            TGRAN16_2   =  8,   // ARMv8.5-GTG Stage-2 Granule 16KB
            TGRAN64_2   =  9,   // ARMv8.5-GTG Stage-2 Granule 64KB
            TGRAN4_2    = 10,   // ARMv8.5-GTG Stage-2 Granule  4KB
            EXS         = 11,   // ARMv8.5-CSEH (1)
            FGT         = 14,   // ARMv8.6-FGT (1)
            ECV         = 15,   // ARMv8.6-ECV (1,2)
            HAFDBS      = 16,   // ARMv8.1-TTHM (1,2)
            VMIDBITS    = 17,   // ARMv8.1-VMID16 (2)
            VH          = 18,   // ARMv8.1-VHE (1)
            HPDS        = 19,   // ARMv8.1-HPD (1), ARMv8.2-TTPBHA (2)
            LOR         = 20,   // ARMv8.1-LOR (1)
            PAN         = 21,   // ARMv8.1-PAN (1), ARMv8.2-ATS1E1 (2)
            SPECSEI     = 22,   // RAS SError on speculative reads/fetches
            XNX         = 23,   // ARMv8.2-TTS2UXN (1)
            TWED        = 24,   // ARMv8.6-TWED (1)
            ETS         = 25,   // ARMv8.0-ETS (1)
            CNP         = 32,   // ARMv8.2-TTCNP (1)
            UAO         = 33,   // ARMv8.2-UAO (1)
            LSM         = 34,   // ARMv8.2-LSMAOC (1)
            IESB        = 35,   // ARMv8.2-IESB (1)
            VARANGE     = 36,   // ARMv8.2-LVA (1)
            CCIDX       = 37,   // ARMv8.3-CCIDX (0,1)
            NV          = 38,   // ARMv8.3-NV (1), ARMv8.4-NV (2)
            ST          = 39,   // ARMv8.4-TTST (1)
            AT          = 40,   // ARMv8.4-LSE (1)
            IDS         = 41,   // ARMv8.4-IDST (1)
            FWB         = 42,   // ARMv8.4-S2FWB (1)
            TTL         = 44,   // ARMv8.4-TTL (1)
            BBM         = 45,   // ARMv8.4-TTRem (0,1,2)
            EVT         = 46,   // ARMv8.1-EVT (1), ARMv8.2-EVT (2)
            E0PD        = 47,   // ARMv8.5-E0PD (1)
        };

        static unsigned id              CPULOCAL;
        static unsigned hazard          CPULOCAL;
        static bool     bsp             CPULOCAL;
        static uint32   affinity        CPULOCAL;
        static uint64   hcr_res0        CPULOCAL;
        static uint64   sctlr32_res0    CPULOCAL;
        static uint64   sctlr32_res1    CPULOCAL;
        static uint64   sctlr64_res0    CPULOCAL;
        static uint64   sctlr64_res1    CPULOCAL;
        static uint64   spsr32_res0     CPULOCAL;
        static uint64   spsr64_res0     CPULOCAL;
        static uint64   tcr32_res0      CPULOCAL;
        static uint64   tcr64_res0      CPULOCAL;
        static uint64   ctr             CPULOCAL;
        static uint64   clidr           CPULOCAL;
        static uint64   ccsidr[7][2]    CPULOCAL;
        static uint64   midr            CPULOCAL;
        static uint64   mpidr           CPULOCAL;
        static uint64   cptr            CPULOCAL;
        static uint64   mdcr            CPULOCAL;

        static uint64   cpu64_feat[2]   CPULOCAL;   // ID_AA64PFRx
        static uint64   dbg64_feat[2]   CPULOCAL;   // ID_AA64DFRx
        static uint64   isa64_feat[2]   CPULOCAL;   // ID_AA64ISARx
        static uint64   mem64_feat[3]   CPULOCAL;   // ID_AA64MMFRx
        static uint64   sve64_feat[1]   CPULOCAL;   // ID_AA64ZFRx

        static uint32   cpu32_feat[3]   CPULOCAL;   // ID_PFRx
        static uint32   dbg32_feat[2]   CPULOCAL;   // ID_DFRx
        static uint32   isa32_feat[7]   CPULOCAL;   // ID_ISARx
        static uint32   mem32_feat[6]   CPULOCAL;   // ID_MMFRx
        static uint32   mfp32_feat[3]   CPULOCAL;   // MVFRx

        static unsigned boot_cpu;
        static unsigned online;
        static Spinlock boot_lock       asm ("__boot_lock");

        ALWAYS_INLINE
        static inline auto remote_affinity (unsigned cpu)
        {
            return __atomic_load_n (reinterpret_cast<decltype (affinity) *>(reinterpret_cast<uintptr_t>(&affinity) - CPU_LOCAL_DATA + CPU_GLOBL_DATA + cpu * PAGE_SIZE), __ATOMIC_RELAXED);
        }

        static inline unsigned feature (Cpu_feature f)
        {
            return cpu64_feat[static_cast<unsigned>(f) / 16] >> static_cast<unsigned>(f) % 16 * 4 & 0xf;
        }

        static inline unsigned feature (Dbg_feature f)
        {
            return dbg64_feat[static_cast<unsigned>(f) / 16] >> static_cast<unsigned>(f) % 16 * 4 & 0xf;
        }

        static inline unsigned feature (Isa_feature f)
        {
            return isa64_feat[static_cast<unsigned>(f) / 16] >> static_cast<unsigned>(f) % 16 * 4 & 0xf;
        }

        static inline unsigned feature (Mem_feature f)
        {
            return mem64_feat[static_cast<unsigned>(f) / 16] >> static_cast<unsigned>(f) % 16 * 4 & 0xf;
        }

        static inline void halt()
        {
            asm volatile ("wfi; msr daifclr, #0xf; msr daifset, #0xf" : : : "memory");
        }

        static inline void preempt_disable()
        {
            asm volatile ("msr daifset, #0xf" : : : "memory");
        }

        static inline void preempt_enable()
        {
            asm volatile ("msr daifclr, #0xf" : : : "memory");
        }

        static void init (unsigned, unsigned);
};
