/*
 * Central Processing Unit (CPU)
 *
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

#include "acpi.hpp"
#include "cache.hpp"
#include "cpu.hpp"
#include "hazard.hpp"
#include "ptab_hpt.hpp"
#include "ptab_npt.hpp"
#include "stdio.hpp"

bool Cpu::bsp;
cpu_t Cpu::id;
unsigned Cpu::hazard;
uint64_t Cpu::res0_hcr, Cpu::res0_hcrx;
uint64_t Cpu::ptab, Cpu::midr, Cpu::mpidr, Cpu::gicr, Cpu::cptr, Cpu::mdcr;
uint64_t Cpu::feat_cpu64[3], Cpu::feat_dbg64[2], Cpu::feat_isa64[4], Cpu::feat_mem64[5], Cpu::feat_sme64[1], Cpu::feat_sve64[1];
uint32_t Cpu::feat_cpu32[3], Cpu::feat_dbg32[2], Cpu::feat_isa32[7], Cpu::feat_mem32[6], Cpu::feat_mfp32[3];

void Cpu::enumerate_features()
{
    uint64_t ctr;

    asm volatile ("mrs %x0, ctr_el0"            : "=r" (ctr));
    asm volatile ("mrs %x0, midr_el1"           : "=r" (midr));
    asm volatile ("mrs %x0, mpidr_el1"          : "=r" (mpidr));

    Cache::init (4 << (ctr >> 16 & BIT_RANGE (3, 0)), 4 << (ctr & BIT_RANGE (3, 0)));

    asm volatile ("mrs %x0, id_aa64pfr0_el1"    : "=r" (feat_cpu64[0]));
    asm volatile ("mrs %x0, id_aa64pfr1_el1"    : "=r" (feat_cpu64[1]));
//  asm volatile ("mrs %x0, id_aa64pfr2_el1"    : "=r" (feat_cpu64[2]));
    asm volatile ("mrs %x0, id_aa64dfr0_el1"    : "=r" (feat_dbg64[0]));
    asm volatile ("mrs %x0, id_aa64dfr1_el1"    : "=r" (feat_dbg64[1]));
    asm volatile ("mrs %x0, id_aa64isar0_el1"   : "=r" (feat_isa64[0]));
    asm volatile ("mrs %x0, id_aa64isar1_el1"   : "=r" (feat_isa64[1]));
    asm volatile ("mrs %x0, id_aa64isar2_el1"   : "=r" (feat_isa64[2]));
//  asm volatile ("mrs %x0, id_aa64isar3_el1"   : "=r" (feat_isa64[3]));
    asm volatile ("mrs %x0, id_aa64mmfr0_el1"   : "=r" (feat_mem64[0]));
    asm volatile ("mrs %x0, id_aa64mmfr1_el1"   : "=r" (feat_mem64[1]));
    asm volatile ("mrs %x0, id_aa64mmfr2_el1"   : "=r" (feat_mem64[2]));
//  asm volatile ("mrs %x0, id_aa64mmfr3_el1"   : "=r" (feat_mem64[3]));
//  asm volatile ("mrs %x0, id_aa64mmfr4_el1"   : "=r" (feat_mem64[4]));
//  asm volatile ("mrs %x0, id_aa64smfr0_el1"   : "=r" (feat_sme64[0]));    // SME
//  asm volatile ("mrs %x0, id_aa64zfr0_el1"    : "=r" (feat_sve64[0]));    // SVE

    if (feature (Cpu_feature::EL1) == 2) {
        asm volatile ("mrs %x0, id_pfr0_el1"    : "=r" (feat_cpu32[0]));
        asm volatile ("mrs %x0, id_pfr1_el1"    : "=r" (feat_cpu32[1]));
//      asm volatile ("mrs %x0, id_pfr2_el1"    : "=r" (feat_cpu32[2]));
        asm volatile ("mrs %x0, id_dfr0_el1"    : "=r" (feat_dbg32[0]));
        asm volatile ("mrs %x0, id_dfr1_el1"    : "=r" (feat_dbg32[1]));
        asm volatile ("mrs %x0, id_isar0_el1"   : "=r" (feat_isa32[0]));
        asm volatile ("mrs %x0, id_isar1_el1"   : "=r" (feat_isa32[1]));
        asm volatile ("mrs %x0, id_isar2_el1"   : "=r" (feat_isa32[2]));
        asm volatile ("mrs %x0, id_isar3_el1"   : "=r" (feat_isa32[3]));
        asm volatile ("mrs %x0, id_isar4_el1"   : "=r" (feat_isa32[4]));
        asm volatile ("mrs %x0, id_isar5_el1"   : "=r" (feat_isa32[5]));
        asm volatile ("mrs %x0, id_isar6_el1"   : "=r" (feat_isa32[6]));
        asm volatile ("mrs %x0, id_mmfr0_el1"   : "=r" (feat_mem32[0]));
        asm volatile ("mrs %x0, id_mmfr1_el1"   : "=r" (feat_mem32[1]));
        asm volatile ("mrs %x0, id_mmfr2_el1"   : "=r" (feat_mem32[2]));
        asm volatile ("mrs %x0, id_mmfr3_el1"   : "=r" (feat_mem32[3]));
        asm volatile ("mrs %x0, id_mmfr4_el1"   : "=r" (feat_mem32[4]));
        asm volatile ("mrs %x0, id_mmfr5_el1"   : "=r" (feat_mem32[5]));
    }

    asm volatile ("mrs %x0, mvfr0_el1"          : "=r" (feat_mfp32[0]));
    asm volatile ("mrs %x0, mvfr1_el1"          : "=r" (feat_mfp32[1]));
    asm volatile ("mrs %x0, mvfr2_el1"          : "=r" (feat_mfp32[2]));

    auto res1_cptr { (feature (Cpu_feature::SME) < 1) * CPTR_TSM    |   // !FEAT_SME
                     (feature (Cpu_feature::SVE) < 1) * CPTR_TZ     |   // !FEAT_SVE
                     CPTR_RES1 };

    auto res0_cptr { (feature (Cpu_feature::AMU) < 1) * CPTR_TAM    |   // !FEAT_AMUv1
                     CPTR_RES0 };

    auto res1_mdcr { MDCR_RES1 };

    auto res0_mdcr { (feature (Dbg_feature::PMSVER)      < 3) *  MDCR_HPMFZS                                    |   // !FEAT_SPEv1p2
                     (feature (Dbg_feature::PMSVER)      < 1) * (MDCR_TPMS | MDCR_E2PB)                         |   // !FEAT_SPE
                     (feature (Dbg_feature::PMUVER)      < 7) *  MDCR_HPMFZO                                    |   // !FEAT_PMUv3p7
                     (feature (Dbg_feature::PMUVER)      < 6) * (MDCR_HLP  | MDCR_HCCD)                         |   // !FEAT_PMUv3p5
                     (feature (Dbg_feature::PMUVER)      < 4) *  MDCR_HPMD                                      |   // !FEAT_PMUv3p1
                     (feature (Dbg_feature::PMUVER)      < 1) * (MDCR_HPME | MDCR_TPM | MDCR_TPMCR | MDCR_HPMN) |   // !FEAT_PMUv3
                     (feature (Dbg_feature::MTPMU)       < 1 ||
                      feature (Cpu_feature::EL3)         > 0) *  MDCR_MTPME                                     |   // !FEAT_MTPMU || EL3
                     (feature (Mem_feature::FGT)         < 1) *  MDCR_TDCC                                      |   // !FEAT_FGT
                     (feature (Dbg_feature::TRACEBUFFER) < 1) *  MDCR_E2TB                                      |   // !FEAT_TRBE
                     (feature (Dbg_feature::TRACEFILT)   < 1) *  MDCR_TTRF                                      |   // !FEAT_TRF
                     MDCR_RES0 };

    cptr = ((hyp1_cptr & ~hyp0_cptr) | res1_cptr) & ~res0_cptr;
    mdcr = ((hyp1_mdcr & ~hyp0_mdcr) | res1_mdcr) & ~res0_mdcr;

    res0_hcr  = (feature (Mem_feature::TWED)      < 1) * (HCR_TWEDEL | HCR_TWEDEn)                  |   // !FEAT_TWED
                (feature (Cpu_feature::MTE)       < 2) * (HCR_TID5 | HCR_DCT | HCR_ATA)             |   // !FEAT_MTE2
                (feature (Mem_feature::EVT)       < 2) * (HCR_TTLBOS | HCR_TTLBIS)                  |   // !FEAT_EVT
                (feature (Mem_feature::EVT)       < 1) * (HCR_TOCU | HCR_TICAB | HCR_TID4)          |   // !FEAT_EVT
                (feature (Cpu_feature::CSV2)      < 2 &&
                 feature (Cpu_feature::CSV2_FRAC) < 2) *  HCR_ENSCXT                                |   // !FEAT_CSV2_2 && !FEAT_CSV2_1p2
                (feature (Cpu_feature::AMU)       < 2) *  HCR_AMVOFFEN                              |   // !FEAT_AMUv1p1
                (feature (Cpu_feature::RME)       < 1) *  HCR_GPF                                   |   // !FEAT_RME
                (feature (Cpu_feature::RAS)       < 2) *  HCR_FIEN                                  |   // !FEAT_RASv1p1
                (feature (Cpu_feature::RAS)       < 1) * (HCR_TEA | HCR_TERR)                       |   // !FEAT_RAS
                (feature (Mem_feature::FWB)       < 1) *  HCR_FWB                                   |   // !FEAT_S2FWB
                (feature (Mem_feature::NV)        < 2) *  HCR_NV2                                   |   // !FEAT_NV2
                (feature (Mem_feature::NV)        < 1) * (HCR_AT | HCR_NV1 | HCR_NV)                |   // !FEAT_NV
                (feature (Isa_feature::API)       < 1 &&
                 feature (Isa_feature::APA)       < 1) * (HCR_API | HCR_APK)                        |   // !FEAT_PAuth
                (feature (Isa_feature::TME)       < 1) *  HCR_TME                                   |   // !FEAT_TME
                (feature (Mem_feature::LO)        < 1) *  HCR_TLOR                                  |   // !FEAT_LOR
                (feature (Mem_feature::VH)        < 1) *  HCR_E2H                                   |   // !FEAT_VHE
                (feature (Cpu_feature::EL3)       > 0) *  HCR_HCD                                   |   //  EL3
                (feature (Cpu_feature::EL0)       < 2) *  HCR_TID0;                                     // !AArch32

    res0_hcrx = (feature (Isa_feature::MOPS)      < 1) * (HCRX_MSCEn | HCRX_MCE2)                   |   // !FEAT_MOPS
                (feature (Mem_feature::CMOW)      < 1) *  HCRX_CMOW                                 |   // !FEAT_CMOW
                (feature (Cpu_feature::NMI)       < 1) * (HCRX_VFNMI | HCRX_VINMI | HCRX_TALLINT)   |   // !FEAT_NMI
                (feature (Cpu_feature::SME)       < 1) *  HCRX_SMPME                                |   // !FEAT_SME
                (feature (Isa_feature::XS)        < 1) * (HCRX_FGTnXS | HCRX_FnXS)                  |   // !FEAT_XS
                (feature (Isa_feature::LS64)      < 3) *  HCRX_EnAS0                                |   // !FEAT_LS64_ACCDATA
                (feature (Isa_feature::LS64)      < 2) *  HCRX_EnASR                                |   // !FEAT_LS64_V
                (feature (Isa_feature::LS64)      < 1) *  HCRX_EnALS;                                   // !FEAT_LS64

    if (!feature (Mem_feature::XNX))
        Npt::xnx = false;
}

void Cpu::init (cpu_t cpu, unsigned e)
{
    if (Acpi::resume)
        hazard = 0;

    else {
        for (auto func { CTORS_L }; func != CTORS_C; (*func++)()) ;

        id   = cpu;
        bsp  = cpu == boot_cpu;

        enumerate_features();
    }

    auto impl { "Unknown" }, part { impl };

    switch (midr >> 24 & BIT_RANGE (7, 0)) {

        case 0x41:
            impl = "Arm";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0xd02: part = "Cortex-A34 (Metis)";            break;
                case 0xd03: part = "Cortex-A53 (Apollo)";           break;
                case 0xd04: part = "Cortex-A35 (Mercury)";          break;
                case 0xd05: part = "Cortex-A55 (Ananke)";           break;
                case 0xd06: part = "Cortex-A65 (Helios)";           break;
                case 0xd07: part = "Cortex-A57 (Atlas)";            break;
                case 0xd08: part = "Cortex-A72 (Maia)";             break;
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
                case 0xd46: part = "Cortex-A510 (Klein)";           break;
                case 0xd47: part = "Cortex-A710 (Matterhorn)";      break;
                case 0xd48: part = "Cortex-X2 (Matterhorn ELP)";    break;
                case 0xd49: part = "Neoverse N2 (Perseus)";         break;
                case 0xd4a: part = "Neoverse E1 (Helios)";          break;
                case 0xd4b: part = "Cortex-A78C (Hercules-C)";      break;
                case 0xd4c: part = "Cortex-X1C (Hera-C)";           break;
                case 0xd4d: part = "Cortex-A715 (Makalu)";          break;
                case 0xd4e: part = "Cortex-X3 (Makalu ELP)";        break;
                case 0xd4f: part = "Neoverse V2 (Demeter)";         break;
                case 0xd80: part = "Cortex-A520 (Hayes)";           break;
                case 0xd81: part = "Cortex-A720 (Hunter)";          break;
                case 0xd82: part = "Cortex-X4 (Hunter ELP)";        break;
                case 0xd83: part = "Neoverse (Poseidon VNAE)";      break;
                case 0xd84: part = "Neoverse (Poseidon V)";         break;
                case 0xd85: part = "Cortex (Blackhawk)";            break;
                case 0xd87: part = "Cortex (Chaberton)";            break;
                case 0xd8a: part = "Cortex (Nevis)";                break;
                case 0xd8b: part = "Cortex (Gelas)";                break;
                case 0xd8c: part = "Cortex (Travis)";               break;
                case 0xd8e: part = "Neoverse (Hermes)";             break;
            }
            break;

        case 0x4e:
            impl = "NVIDIA";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0x003: part = "Denver";                        break;
                case 0x004: part = "Carmel";                        break;
            }
            break;

        case 0x51:
            impl = "Qualcomm";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0x800: part = "Kryo 2xx Gold";                 break;
                case 0x801: part = "Kryo 2xx Silver";               break;
                case 0x802: part = "Kryo 3xx Gold";                 break;
                case 0x803: part = "Kryo 3xx Silver";               break;
                case 0x804: part = "Kryo 4xx Gold";                 break;
                case 0x805: part = "Kryo 4xx Silver";               break;
                case 0xc00: part = "Falkor";                        break;
                case 0xc01: part = "Saphira";                       break;
            }
            break;

        case 0x61:
            impl = "Apple";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0x001: part = "A7 (Cyclone)";                  break;
                case 0x002: part = "A8 (Typhoon)";                  break;
                case 0x003: part = "A8X (Typhoon)";                 break;
                case 0x004: part = "A9 (Twister)";                  break;
                case 0x005: part = "A9X (Twister)";                 break;
                case 0x006: part = "A10 (Hurricane)";               break;
                case 0x007: part = "A10X (Hurricane)";              break;
                case 0x008: part = "A11 (Monsoon)";                 break;
                case 0x009: part = "A11 (Mistral)";                 break;
                case 0x00b: part = "A12 (Vortex)";                  break;
                case 0x00c: part = "A12 (Tempest)";                 break;
                case 0x010: part = "A12X (Vortex)";                 break;
                case 0x011: part = "A12X (Tempest)";                break;
                case 0x012: part = "A13 (Lightning)";               break;
                case 0x013: part = "A13 (Thunder)";                 break;
                case 0x022: part = "M1 (Icestorm)";                 break;
                case 0x023: part = "M1 (Firestorm)";                break;
                case 0x024: part = "M1 Pro (Icestorm)";             break;
                case 0x025: part = "M1 Pro (Firestorm)";            break;
                case 0x028: part = "M1 Max (Icestorm)";             break;
                case 0x029: part = "M1 Max (Firestorm)";            break;
            }
            break;
    }

    trace (TRACE_CPU, "CORE: %02lu:%02lu:%02lu:%02lu %s %s r%lup%lu PA:%u XNX:%u GIC:%u (EL%u)",
           mpidr >> 32 & BIT_RANGE (7, 0), mpidr >> 16 & BIT_RANGE (7, 0), mpidr >> 8 & BIT_RANGE (7, 0), mpidr & BIT_RANGE (7, 0),
           impl, part, midr >> 20 & BIT_RANGE (3, 0), midr & BIT_RANGE (3, 0),
           feature (Mem_feature::PARANGE), feature (Mem_feature::XNX), feature (Cpu_feature::GIC), e);

    Nptp::init();

    boot_lock.unlock();
}

void Cpu::fini()
{
    auto const s { Acpi::get_transition() };

    Acpi::fini (s);
}

void Cpu::set_vmm_regs (uintptr_t (&x)[31], uint64_t &hcr, uint64_t &vpidr, uint64_t &vmpidr, uint32_t &elrsr)
{
    x[0]  = feat_cpu64[0]; x[1] = feat_cpu64[1];
    x[2]  = feat_dbg64[0]; x[3] = feat_dbg64[1];
    x[4]  = feat_isa64[0]; x[5] = feat_isa64[1]; x[6] = feat_isa64[2];
    x[7]  = feat_mem64[0]; x[8] = feat_mem64[1]; x[9] = feat_mem64[2];
    x[10] = feat_sme64[0]; x[11] = feat_sve64[0];

    x[16] = static_cast<uint64_t>(feat_cpu32[1]) << 32 | feat_cpu32[0];
    x[17] = static_cast<uint64_t>(feat_dbg32[0]) << 32 | feat_cpu32[2];
    x[18] = static_cast<uint64_t>(feat_isa32[0]) << 32 | feat_dbg32[1];
    x[19] = static_cast<uint64_t>(feat_isa32[2]) << 32 | feat_isa32[1];
    x[20] = static_cast<uint64_t>(feat_isa32[4]) << 32 | feat_isa32[3];
    x[21] = static_cast<uint64_t>(feat_isa32[6]) << 32 | feat_isa32[5];
    x[22] = static_cast<uint64_t>(feat_mem32[1]) << 32 | feat_mem32[0];
    x[23] = static_cast<uint64_t>(feat_mem32[3]) << 32 | feat_mem32[2];
    x[24] = static_cast<uint64_t>(feat_mem32[5]) << 32 | feat_mem32[4];

    x[29] = static_cast<uint64_t>(feat_mfp32[1]) << 32 | feat_mfp32[0];
    x[30] =                                              feat_mfp32[2];

    hcr    = constrain_hcr (0);
    vpidr  = midr;
    vmpidr = mpidr;
    elrsr  = 0;
}

bool Cpu::allocate (cpu_t cpu, uint64_t m, uint64_t r)
{
    auto const c { Buddy::alloc (0, Buddy::Fill::BITS0) };  // CPU-Local Data
    auto const d { Buddy::alloc (0, Buddy::Fill::BITS0) };  // Data Stack

    if (EXPECT_TRUE (c && d)) {

        Hptp hptp { 0 };

        // Share kernel code and data
        hptp.share_from_master (LINK_ADDR);

        // Map cpu-local data and stack
        hptp.update (MMAP_CPU_DATA, Kmem::ptr_to_phys (c), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram());
        hptp.update (MMAP_CPU_DSTK, Kmem::ptr_to_phys (d), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram());

        // Add to CPU array
        Hptp::master_map (MMAP_GLB_CPUS + cpu * PAGE_SIZE (0), Kmem::ptr_to_phys (c), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram());

        // Prefill addresses
        *Kmem::loc_to_glob (cpu, &mpidr) = m;
        *Kmem::loc_to_glob (cpu, &gicr)  = r;
        *Kmem::loc_to_glob (cpu, &ptab)  = hptp.root_addr();

        return true;
    }

    Buddy::free (c);
    Buddy::free (d);

    return false;
}
