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

    public:
        enum class Cpu_feature  // Appendix K13
        {
            EL0         =  0,   // EL0 Support AArch64 only (1), AArch64+AArch32 (2)
            EL1         =  1,   // EL1 Support AArch64 only (1), AArch64+AArch32 (2)
            EL2         =  2,   // EL2 Support AArch64 only (1), AArch64+AArch32 (2)
            EL3         =  3,   // EL3 Support AArch64 only (1), AArch64+AArch32 (2)
            FP          =  4,   // Floating Point (0), FEAT_FP16 (1), Unsupported (15)
            ADVSIMD     =  5,   // Advanced SIMD (0), FEAT_FP16 (1), Unsupported (15)
            GIC         =  6,   // GIC System Registers v3.0/4.0 (1), v4.1 (3)
            RAS         =  7,   // FEAT_RASv1 (1), FEAT_RASv1p1 (2)
            SVE         =  8,   // FEAT_SVE (1)
            SEL2        =  9,   // FEAT_SEL2 (1)
            MPAM        = 10,   // FEAT_MPAM (1)
            AMU         = 11,   // FEAT_AMUv1 (1), FEAT_AMUv1p1 (2)
            DIT         = 12,   // FEAT_DIT (1)
            CSV2        = 14,   // FEAT_CSV2 (1,2)
            CSV3        = 15,   // FEAT_CSV3 (1)
            BT          = 16,   // FEAT_BTI (1)
            SSBS        = 17,   // FEAT_SSBS (1,2)
            MTE         = 18,   // FEAT_MTE (1,2)
            RAS_FRAC    = 19,   // FEAT_RAS_frac (1)
            MPAM_FRAC   = 20,   // FRAM_MPAM_frac (1)
        };

        enum class Dbg_feature  // Appendix K13
        {
            DEBUGVER    =  0,   // FEAT_Debugv8p0 (6), FEAT_Debugv8p1 (7), FEAT_Debugv8p2 (8), FEAT_Debugv8p4 (9)
            TRACEVER    =  1,   // Trace Support
            PMUVER      =  2,   // FEAT_PMUv3 (1), FEAT_PMUv3p1 (4), FEAT_PMUv3p4 (5), FEAT_PMUv3p5 (6), FEAT_PMUv3p7 (7)
            BRPS        =  3,   // Number of Breakpoints - 1
            WRPS        =  5,   // Number of Watchpoints - 1
            CTXCMPS     =  7,   // Number of Context-Aware Breakpoints - 1
            PMSVER      =  8,   // FEAT_SPE (1), FEAT_SPEv1p1 (2), FEAT_SPEv1p2 (3)
            DOUBLELOCK  =  9,   // FEAT_DoubleLock (0), Unsupported (15)
            TRACEFILT   = 10,   // FEAT_TRF (1)
            TRACEBUFFER = 11,   // FEAT_TRBE (1)
            MTPMU       = 12,   // FEAT_MTPMU (1)
            BRBE        = 13,   // FEAT_BRBE (1)
            CSRE        = 14,   // FEAT_CSRE (1)
        };

        enum class Isa_feature  // Appendix K13
        {
            AES         =  1,   // FEAT_AES (1), FEAT_PMULL (2)
            SHA1        =  2,   // FEAT_SHA1 (1)
            SHA2        =  3,   // FEAT_SHA256 (1), FEAT_SHA512 (2)
            CRC32       =  4,   // FEAT_CRC (1)
            ATOMIC      =  5,   // FEAT_LSE (2)
            TME         =  6,   // FEAT_TME (1)
            RDM         =  7,   // FEAT_RDM (1)
            SHA3        =  8,   // FEAT_SHA3 (1)
            SM3         =  9,   // FEAT_SM3 (1)
            SM4         = 10,   // FEAT_SM4 (1)
            DP          = 11,   // FEAT_DotProd (1)
            FHM         = 12,   // FEAT_FHM (1)
            TS          = 13,   // FEAT_FlagM (1), FEAT_FlagM2 (2)
            TLB         = 14,   // FEAT_TLBIOS (1,2), FEAT_TLBIRANGE (2)
            RNDR        = 15,   // FEAT_RNG (1)
            DPB         = 16,   // FEAT_DPB (1), FEAT_DPB2 (2)
            APA         = 17,   // FEAT_PAuth (0,1,2), FEAT_PAuth2 (3), FEAT_FPAC (4,5)
            API         = 18,   // FEAT_PAuth (0,1,2), FEAT_PAuth2 (3), FEAT_FPAC (4,5)
            JSCVT       = 19,   // FEAT_JSCVT (1)
            FCMA        = 20,   // FEAT_FCMA (1)
            LRCPC       = 21,   // FEAT_LRCPC (1), FEAT_LRCPC2 (2)
            GPA         = 22,   // FEAT_PAuth
            GPI         = 23,   // FEAT_PAuth
            FRINTTS     = 24,   // FEAT_FRINTTS (1)
            SB          = 25,   // FEAT_SB (1)
            SPECRES     = 26,   // FEAT_SPECRES (1)
            BF16        = 27,   // FEAT_BF16 (1)
            DGH         = 28,   // FEAT_DGH (1)
            I8MM        = 29,   // FEAT_I8MM (1)
            XS          = 30,   // FEAT_XS (1)
            LS64        = 31,   // FEAT_LS64 (1), FEAT_LS64_V (2), FEAT_LS64_ACCDATA (3)
            WFXT        = 32,   // FEAT_WFXT (1)
            RPRES       = 33,   // FEAT_RPRES (1)
        };

        enum class Mem_feature  // Appendix K13
        {
            PARANGE     =  0,   // FEAT_LPA (6)
            ASIDBITS    =  1,   // Number of ASID Bits
            BIGEND      =  2,   // Mixed Endian Support
            SNSMEM      =  3,   // Secure/Non-Secure Memory Distinction
            BIGEND_EL0  =  4,   // Mixed Endian Support at EL0
            TGRAN16     =  5,   // Stage-1 Granule 16KB
            TGRAN64     =  6,   // Stage-1 Granule 64KB
            TGRAN4      =  7,   // Stage-1 Granule  4KB
            TGRAN16_2   =  8,   // FEAT_GTG Stage-2 Granule 16KB
            TGRAN64_2   =  9,   // FEAT_GTG Stage-2 Granule 64KB
            TGRAN4_2    = 10,   // FEAT_GTG Stage-2 Granule  4KB
            EXS         = 11,   // FEAT_ExS (1)
            FGT         = 14,   // FEAT_FGT (1)
            ECV         = 15,   // FEAT_ECV (1,2)
            HAFDBS      = 16,   // FEAT_HAFDBS (1,2)
            VMIDBITS    = 17,   // FEAT_VMID16 (2)
            VH          = 18,   // FEAT_VHE (1)
            HPDS        = 19,   // FEAT_HPDS (1), FEAT_HPDS2 (2)
            LO          = 20,   // FEAT_LOR (1)
            PAN         = 21,   // FEAT_PAN (1), FEAT_PAN2 (2), FEAT_PAN3 (3)
            SPECSEI     = 22,   // RAS SError on speculative reads/fetches
            XNX         = 23,   // FEAT_XNX (1)
            TWED        = 24,   // FEAT_TWED (1)
            ETS         = 25,   // FEAT_ETS (1)
            HCX         = 26,   // FEAT_HCX (1)
            AFP         = 27,   // FEAT_AFP (1)
            CNP         = 32,   // FEAT_TTCNP (1)
            UAO         = 33,   // FEAT_UAO (1)
            LSM         = 34,   // FEAT_LSMAOC (1)
            IESB        = 35,   // FEAT_IESB (1)
            VARANGE     = 36,   // FEAT_LVA (1)
            CCIDX       = 37,   // FEAT_CCIDX (1)
            NV          = 38,   // FEAT_NV (1), FEAT_NV2 (2)
            ST          = 39,   // FEAT_TTST (1)
            AT          = 40,   // FEAT_LSE2 (1)
            IDS         = 41,   // FEAT_IDST (1)
            FWB         = 42,   // FEAT_S2FWB (1)
            TTL         = 44,   // FEAT_TTL (1)
            BBM         = 45,   // FEAT_BBM (0,1,2)
            EVT         = 46,   // FEAT_EVT (1,2)
            E0PD        = 47,   // FEAT_E0PD (1)
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
        static uint64   midr            CPULOCAL;
        static uint64   mpidr           CPULOCAL;
        static uint64   cptr            CPULOCAL;
        static uint64   mdcr            CPULOCAL;

        static uint64   cpu64_feat[2]   CPULOCAL;   // ID_AA64PFRx
        static uint64   dbg64_feat[2]   CPULOCAL;   // ID_AA64DFRx
        static uint64   isa64_feat[3]   CPULOCAL;   // ID_AA64ISARx
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
