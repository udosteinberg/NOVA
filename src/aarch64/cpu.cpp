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

#include "arch.hpp"
#include "cache.hpp"
#include "cpu.hpp"
#include "extern.hpp"
#include "gicc.hpp"
#include "gicd.hpp"
#include "gicr.hpp"
#include "stdio.hpp"

unsigned Cpu::id, Cpu::hazard, Cpu::boot_cpu, Cpu::online;
bool Cpu::bsp;
uint32 Cpu::affinity;
uint64 Cpu::hcr_res0;
uint64 Cpu::sctlr32_res0, Cpu::sctlr32_res1, Cpu::sctlr64_res0, Cpu::sctlr64_res1;
uint64 Cpu::spsr32_res0, Cpu::spsr64_res0, Cpu::tcr32_res0, Cpu::tcr64_res0;
uint64 Cpu::midr, Cpu::mpidr, Cpu::cptr, Cpu::mdcr;
uint64 Cpu::cpu64_feat[2], Cpu::dbg64_feat[2], Cpu::isa64_feat[3], Cpu::mem64_feat[3], Cpu::sve64_feat[1];
uint32 Cpu::cpu32_feat[3], Cpu::dbg32_feat[2], Cpu::isa32_feat[7], Cpu::mem32_feat[6], Cpu::mfp32_feat[3];
Spinlock Cpu::boot_lock;

void Cpu::enumerate_features()
{
    asm volatile ("mrs %0, midr_el1"            : "=r" (midr));
    asm volatile ("mrs %0, mpidr_el1"           : "=r" (mpidr));

    asm volatile ("mrs %0, id_aa64dfr0_el1"     : "=r" (dbg64_feat[0]));
    asm volatile ("mrs %0, id_aa64dfr1_el1"     : "=r" (dbg64_feat[1]));
    asm volatile ("mrs %0, id_aa64isar0_el1"    : "=r" (isa64_feat[0]));
    asm volatile ("mrs %0, id_aa64isar1_el1"    : "=r" (isa64_feat[1]));
//  asm volatile ("mrs %0, id_aa64isar2_el1"    : "=r" (isa64_feat[2]));    // ARMv8.7
    asm volatile ("mrs %0, id_aa64mmfr0_el1"    : "=r" (mem64_feat[0]));
    asm volatile ("mrs %0, id_aa64mmfr1_el1"    : "=r" (mem64_feat[1]));
//  asm volatile ("mrs %0, id_aa64mmfr2_el1"    : "=r" (mem64_feat[2]));    // ARMv8.2
    asm volatile ("mrs %0, id_aa64pfr0_el1"     : "=r" (cpu64_feat[0]));
    asm volatile ("mrs %0, id_aa64pfr1_el1"     : "=r" (cpu64_feat[1]));
//  asm volatile ("mrs %0, id_aa64zfr0_el1"     : "=r" (sve64_feat[0]));    // SVE

    if (feature (Cpu_feature::EL1) == 2) {
        asm volatile ("mrs %x0, id_dfr0_el1"    : "=r" (dbg32_feat[0]));
//      asm volatile ("mrs %x0, id_dfr1_el1"    : "=r" (dbg32_feat[1]));    // ARMv8.6
        asm volatile ("mrs %x0, id_isar0_el1"   : "=r" (isa32_feat[0]));
        asm volatile ("mrs %x0, id_isar1_el1"   : "=r" (isa32_feat[1]));
        asm volatile ("mrs %x0, id_isar2_el1"   : "=r" (isa32_feat[2]));
        asm volatile ("mrs %x0, id_isar3_el1"   : "=r" (isa32_feat[3]));
        asm volatile ("mrs %x0, id_isar4_el1"   : "=r" (isa32_feat[4]));
        asm volatile ("mrs %x0, id_isar5_el1"   : "=r" (isa32_feat[5]));
//      asm volatile ("mrs %x0, id_isar6_el1"   : "=r" (isa32_feat[6]));    // ARMv8.2
        asm volatile ("mrs %x0, id_mmfr0_el1"   : "=r" (mem32_feat[0]));
        asm volatile ("mrs %x0, id_mmfr1_el1"   : "=r" (mem32_feat[1]));
        asm volatile ("mrs %x0, id_mmfr2_el1"   : "=r" (mem32_feat[2]));
        asm volatile ("mrs %x0, id_mmfr3_el1"   : "=r" (mem32_feat[3]));
        asm volatile ("mrs %x0, id_mmfr4_el1"   : "=r" (mem32_feat[4]));
//      asm volatile ("mrs %x0, id_mmfr5_el1"   : "=r" (mem32_feat[5]));
        asm volatile ("mrs %x0, id_pfr0_el1"    : "=r" (cpu32_feat[0]));
        asm volatile ("mrs %x0, id_pfr1_el1"    : "=r" (cpu32_feat[1]));
//      asm volatile ("mrs %x0, id_pfr2_el1"    : "=r" (cpu32_feat[2]));
    }

    asm volatile ("mrs %x0, mvfr0_el1"          : "=r" (mfp32_feat[0]));
    asm volatile ("mrs %x0, mvfr1_el1"          : "=r" (mfp32_feat[1]));
    asm volatile ("mrs %x0, mvfr2_el1"          : "=r" (mfp32_feat[2]));
}

void Cpu::init (unsigned cpu, unsigned e)
{
    for (void (**func)() = &CTORS_L; func != &CTORS_C; (*func++)()) ;

    id  = cpu;
    bsp = cpu == boot_cpu;

    enumerate_features();

    char const *impl = "Unknown", *part = impl;

    switch (midr >> 24 & 0xff) {

        case 0x41:
            impl = "ARM Limited";
            switch (midr >> 4 & 0xfff) {
                case 0xd02: part = "Cortex-A34";                    break;
                case 0xd03: part = "Cortex-A53 (Apollo)";           break;
                case 0xd04: part = "Cortex-A35 (Mercury)";          break;
                case 0xd05: part = "Cortex-A55 (Ananke)";           break;
                case 0xd06: part = "Cortex-A65 (Helios)";           break;
                case 0xd07: part = "Cortex-A57 (Atlas)";            break;
                case 0xd08: part = "Cortex-A72 (Maya)";             break;
                case 0xd09: part = "Cortex-A73 (Artemis)";          break;
                case 0xd0a: part = "Cortex-A75 (Prometheus)";       break;
                case 0xd0b: part = "Cortex-A76 (Enyo)";             break;
                case 0xd0c: part = "Neoverse N1 (Ares)";            break;
                case 0xd0d: part = "Cortex-A77 (Deimos)";           break;
                case 0xd0e: part = "Cortex-A76AE (Enyo-AE)";        break;
                case 0xd40: part = "Neoverse V1 (Zeus)";            break;
                case 0xd41: part = "Cortex-A78 (Hercules)";         break;
                case 0xd42: part = "Cortex-A78AE (Hercules-AE)";    break;
                case 0xd43: part = "Cortex-A65AE (Helios-AE)";      break;
                case 0xd44: part = "Cortex-X1 (Hera)";              break;
                case 0xd46: part = "Cortex-A (Klein)";              break;
                case 0xd47: part = "Cortex-A (Matterhorn)";         break;
                case 0xd49: part = "Neoverse N2 (Perseus)";         break;
                case 0xd4a: part = "Neoverse E1 (Helios)";          break;
            }
            break;

        case 0x51:
            impl = "Qualcomm Inc.";
            switch (midr >> 4 & 0xfff) {
                case 0x802: part = "Kryo 3xx Gold";                 break;
                case 0x803: part = "Kryo 3xx Silver";               break;
                case 0x804: part = "Kryo 4xx Gold";                 break;
                case 0x805: part = "Kryo 4xx Silver";               break;
                case 0xc00: part = "Falkor";                        break;
                case 0xc01: part = "Saphira";                       break;
            }
            break;
    }

    affinity = static_cast<uint32>((mpidr >> 8 & 0xff000000) | (mpidr & 0xffffff));

    trace (TRACE_CPU, "CORE: %x:%x:%x %s %20s r%llup%llu PA:%u GIC:%u (EL%u)",
           affinity >> 16 & 0xff, affinity >> 8 & 0xff, affinity & 0xff,
           impl, part, midr >> 20 & 0xf, midr & 0xf, feature (Mem_feature::PARANGE), feature (Cpu_feature::GIC), e);

    uint64 cptr_res1 =  (feature (Cpu_feature::FP)      != 15 ||
                         feature (Cpu_feature::ADVSIMD) != 15 ? 0 : CPTR_TFP)   |
                        (feature (Cpu_feature::SVE)     >=  1 ? 0 : CPTR_TZ)    |
                        CPTR_RES1;

    uint64 cptr_res0 =  (feature (Cpu_feature::AMU)     >=  1 ? 0 : CPTR_TAM)   |
                        CPTR_RES0;

    uint64 mdcr_res1 =  MDCR_RES1;

    uint64 mdcr_res0 =  (feature (Dbg_feature::PMSVER)      >= 3 ? 0 : MDCR_HPMFZS)                                     |
                        (feature (Dbg_feature::PMSVER)      >= 2 ? 0 : MDCR_TPMS | MDCR_E2PB)                           |
                        (feature (Dbg_feature::PMUVER)      >= 7 ? 0 : MDCR_HPMFZO)                                     |
                        (feature (Dbg_feature::PMUVER)      >= 6 ? 0 : MDCR_HLP | MDCR_HCCD)                            |
                        (feature (Dbg_feature::PMUVER)      >= 4 ? 0 : MDCR_HPMD)                                       |
                        (feature (Dbg_feature::PMUVER)      >= 1 ? 0 : MDCR_HPME | MDCR_TPM | MDCR_TPMCR | MDCR_HPMN)   |
                        (feature (Dbg_feature::MTPMU)       >= 1 ? 0 : MDCR_MTPME)                                      |
                        (feature (Mem_feature::FGT)         >= 1 ? 0 : MDCR_TDCC)                                       |
                        (feature (Dbg_feature::TRACEBUFFER) >= 1 ? 0 : MDCR_E2TB)                                       |
                        (feature (Dbg_feature::TRACEFILT)   >= 1 ? 0 : MDCR_TTRF)                                       |
                        MDCR_RES0;

    cptr = cptr_res1 & ~cptr_res0;
    mdcr = mdcr_res1 & ~mdcr_res0;

    hcr_res0  = (feature (Mem_feature::TWED) >= 1 ? 0 : HCR_TWEDEL | HCR_TWEDEn)            |
                (feature (Cpu_feature::MTE)  >= 2 ? 0 : HCR_TID5 | HCR_DCT | HCR_ATA)       |
                (feature (Mem_feature::EVT)  >= 2 ? 0 : HCR_TTLBOS | HCR_TTLBIS)            |
                (feature (Mem_feature::EVT)  >= 1 ? 0 : HCR_TOCU | HCR_TICAB | HCR_TID4)    |
                (feature (Cpu_feature::CSV2) >= 2 ? 0 : HCR_ENSCXT)                         |
                (feature (Cpu_feature::AMU)  >= 2 ? 0 : HCR_AMVOFFEN)                       |
                (feature (Cpu_feature::RAS)  >= 2 ? 0 : HCR_FIEN)                           |
                (feature (Cpu_feature::RAS)  >= 1 ? 0 : HCR_TEA | HCR_TERR)                 |
                (feature (Mem_feature::FWB)  >= 1 ? 0 : HCR_FWB)                            |
                (feature (Mem_feature::NV)   >= 2 ? 0 : HCR_NV2)                            |
                (feature (Mem_feature::NV)   >= 1 ? 0 : HCR_AT | HCR_NV1 | HCR_NV)          |
                (feature (Isa_feature::APA)  >= 1 ||
                 feature (Isa_feature::API)  >= 1 ||
                 feature (Isa_feature::GPA)  >= 1 ||
                 feature (Isa_feature::GPI)  >= 1 ? 0 : HCR_API | HCR_APK)                  |
                (feature (Mem_feature::LO)   >= 1 ? 0 : HCR_TLOR)                           |
                (feature (Mem_feature::VH)   >= 1 ? 0 : HCR_E2H)                            |
                (feature (Isa_feature::TLB)  >= 1 ? 0 : HCR_TTLB)                           |
                (feature (Isa_feature::DPB)  >= 1 ? 0 : HCR_TPCP)                           |
                HCR_RES0;

    sctlr32_res1 = (feature (Mem_feature::LSM)     >= 1 ? 0 : SCTLR_A32_LSMAOE | SCTLR_A32_nTLSMD)  |
                   (feature (Mem_feature::PAN)     >= 1 ? 0 : SCTLR_ALL_SPAN)                       |
                   SCTLR_A32_RES1;

    sctlr32_res0 = (feature (Cpu_feature::SSBS)    >= 1 ? 0 : SCTLR_A32_DSSBS)  |
                   (feature (Isa_feature::SPECRES) >= 1 ? 0 : SCTLR_ALL_EnRCTX) |
                   SCTLR_A32_RES0;

    sctlr64_res1 = (feature (Mem_feature::LSM)     >= 1 ? 0 : SCTLR_A64_LSMAOE | SCTLR_A64_nTLSMD)  |
                   (feature (Mem_feature::PAN)     >= 1 ? 0 : SCTLR_ALL_SPAN)                       |
                   (feature (Mem_feature::EXS)     >= 1 ? 0 : SCTLR_A64_EIS | SCTLR_A64_EOS)        |
                   (feature (Cpu_feature::CSV2)    >= 2 ? 0 : SCTLR_A64_TSCXT)                      |
                   SCTLR_A64_RES1;

    sctlr64_res0 = (feature (Dbg_feature::CSRE)    >= 1 ? 0 : SCTLR_A64_EnCSR0)                                                                     |
                   (feature (Mem_feature::PAN)     >= 3 ? 0 : SCTLR_A64_EPAN)                                                                       |
                   (feature (Isa_feature::LS64)    >= 1 ? 0 : SCTLR_A64_EnALS | SCTLR_A64_EnAS0 | SCTLR_A64_EnASR)                                  |
                   (feature (Isa_feature::TME)     >= 1 ? 0 : SCTLR_A64_TME | SCTLR_A64_TME0 | SCTLR_A64_TMT | SCTLR_A64_TMT0)                      |
                   (feature (Mem_feature::TWED)    >= 1 ? 0 : SCTLR_A64_TWEDEL | SCTLR_A64_TWEDEn)                                                  |
                   (feature (Cpu_feature::SSBS)    >= 1 ? 0 : SCTLR_A64_DSSBS)                                                                      |
                   (feature (Cpu_feature::MTE)     >= 2 ? 0 : SCTLR_A64_ATA | SCTLR_A64_ATA0 | SCTLR_A64_TCF | SCTLR_A64_TCF0 | SCTLR_A64_ITFSB)    |
                   (feature (Cpu_feature::BT)      >= 1 ? 0 : SCTLR_A64_BT1 | SCTLR_A64_BT0)                                                        |
                   (feature (Isa_feature::APA)     >= 1 ||
                    feature (Isa_feature::API)     >= 1 ||
                    feature (Isa_feature::GPA)     >= 1 ||
                    feature (Isa_feature::GPI)     >= 1 ? 0 : SCTLR_A64_EnIA | SCTLR_A64_EnIB | SCTLR_A64_EnDA | SCTLR_A64_EnDB)                    |
                   (feature (Mem_feature::IESB)    >= 1 ? 0 : SCTLR_A64_IESB)                                                                       |
                   (feature (Isa_feature::SPECRES) >= 1 ? 0 : SCTLR_ALL_EnRCTX)                                                                     |
                   (feature (Mem_feature::AT)      >= 1 ? 0 : SCTLR_A64_nAA)                                                                        |
                   SCTLR_A64_RES0;

    spsr32_res0 = (feature (Cpu_feature::DIT)  >= 1 ? 0 : SPSR_ALL_DIT)   |
                  (feature (Cpu_feature::SSBS) >= 1 ? 0 : SPSR_A32_SSBS)  |
                  (feature (Mem_feature::PAN)  >= 1 ? 0 : SPSR_ALL_PAN)   |
                  SPSR_A32_RES0;

    spsr64_res0 = (feature (Dbg_feature::CSRE) >= 1 ? 0 : SPSR_A64_CSR)   |
                  (feature (Cpu_feature::MTE)  >= 2 ? 0 : SPSR_A64_TCO)   |
                  (feature (Cpu_feature::DIT)  >= 1 ? 0 : SPSR_ALL_DIT)   |
                  (feature (Mem_feature::UAO)  >= 1 ? 0 : SPSR_A64_UAO)   |
                  (feature (Mem_feature::PAN)  >= 1 ? 0 : SPSR_ALL_PAN)   |
                  (feature (Cpu_feature::SSBS) >= 1 ? 0 : SPSR_A64_SSBS)  |
                  (feature (Cpu_feature::BT)   >= 1 ? 0 : SPSR_A64_BTYPE) |
                  SPSR_A64_RES0;

    tcr32_res0 = (feature (Mem_feature::HPDS)   >= 2 ? 0 : TCR_ALL_HWU162 | TCR_ALL_HWU161 | TCR_ALL_HWU160 | TCR_ALL_HWU159 | TCR_ALL_HWU062 | TCR_ALL_HWU061 | TCR_ALL_HWU060 | TCR_ALL_HWU059) |
                 (feature (Mem_feature::HPDS)   >= 1 ? 0 : TCR_ALL_HPD1  | TCR_ALL_HPD0)    |
                 TCR_A32_RES0;

    tcr64_res0 = (feature (Mem_feature::PARANGE) >= 6 ? 0 : TCR_A64_DS) |
                 (feature (Cpu_feature::MTE)     >= 2 ? 0 : TCR_A64_TCMA1 | TCR_A64_TCMA0)   |
                 (feature (Mem_feature::E0PD)    >= 1 ? 0 : TCR_A64_E0PD1 | TCR_A64_E0PD0)   |
                 (feature (Cpu_feature::SVE)     >= 1 ? 0 : TCR_A64_NFD1  | TCR_A64_NFD0)    |
                 (feature (Isa_feature::APA)     >= 1 ||
                  feature (Isa_feature::API)     >= 1 ||
                  feature (Isa_feature::GPA)     >= 1 ||
                  feature (Isa_feature::GPI)     >= 1 ? 0 : TCR_A64_TBID1 | TCR_A64_TBID0)   |
                 (feature (Mem_feature::HPDS)    >= 2 ? 0 : TCR_ALL_HWU162 | TCR_ALL_HWU161 | TCR_ALL_HWU160 | TCR_ALL_HWU159 | TCR_ALL_HWU062 | TCR_ALL_HWU061 | TCR_ALL_HWU060 | TCR_ALL_HWU059) |
                 (feature (Mem_feature::HPDS)    >= 1 ? 0 : TCR_ALL_HPD1  | TCR_ALL_HPD0)    |
                 (feature (Mem_feature::HAFDBS)  >= 2 ? 0 : TCR_A64_HD)                      |
                 (feature (Mem_feature::HAFDBS)  >= 1 ? 0 : TCR_A64_HA)                      |
                 TCR_A64_RES0;

    Cache::init();

    Gicd::init();
    Gicr::init();
    Gicc::init();

    boot_lock.unlock();
}
