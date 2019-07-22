/*
 * Virtual Machine Control Block (VMCB)
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

#include "buddy.hpp"
#include "cpu.hpp"
#include "types.hpp"

class Vmcb
{
    public:
        static constexpr uint64 hcr_hyp0 =  HCR_ATA         |   // Trap GCR, RGSR, TFSR*
                                            HCR_ENSCXT      |   // Trap SCXTNUM
                                            HCR_FIEN        |   // Trap ERXPFG*
                                            HCR_NV2         |
                                            HCR_NV1         |
                                            HCR_NV          |
                                            HCR_APK         |   // Trap APDAKey*, APDBKey*, APGAKey*, APIAKey*, APIBKey*
                                            HCR_E2H         |
                                            HCR_ID          |
                                            HCR_CD          |
                                            HCR_TGE         |
                                            HCR_DC;

        static constexpr uint64 hcr_hyp1 =  HCR_TID5        |   // Trap GMID
                                            HCR_TID4        |   // Trap CCSIDR*, CLIDR, CSSELR
                                            HCR_TERR        |   // Trap ERRIDR, ERRSELR, ERXADDR, ERXCTLR, ERXFR, ERXMISC*, ERXSTATUS
                                            HCR_TLOR        |   // Trap LORC, LOREA, LORID, LORN, LORSA
                                            HCR_TSW         |   // Trap DC ISW/CSW/CISW
                                            HCR_TACR        |   // Trap ACTLR
                                            HCR_TIDCP       |   // Trap S3_* implementation defined registers
                                            HCR_TSC         |   // Trap SMC
                                            HCR_TID3        |   // Trap ID_AA64AFR*,ID_AA64DFR*, ID_AA64ISAR*, ID_AA64MMFR*, ID_AA64PFR*, ID_AA64ZFR*, ID_AFR0, ID_DFR0, ID_ISAR*, ID_MMFR, ID_PFR, MVFR*
                                            HCR_TID2        |   // Trap CCSIDR2, CCSIDR, CLIDR, CSSELR, CTR_EL0
                                            HCR_TID1        |   // Trap AIDR, REVIDR
                                            HCR_TID0        |   // Trap JIDR
                                            HCR_TWE         |   // Trap WFE
                                            HCR_TWI         |   // Trap WFI
                                            HCR_BSU_INNER   |
                                            HCR_FB          |
                                            HCR_AMO         |
                                            HCR_IMO         |
                                            HCR_FMO         |
                                            HCR_PTW         |
                                            HCR_SWIO        |
                                            HCR_VM;

        static constexpr uint64 cptr_hyp0 = 0;

        static constexpr uint64 cptr_hyp1 = CPTR_TAM        |   // Trap activity monitor: AMCFGR, AMCGCR, AMCNTEN*, AMCR, AMEVCNTR*, AMEVTYPER*, AMUSERENR
                                            CPTR_TTA        |   // Trap trace registers
                                            CPTR_TZ;            // Trap ZCR

        static constexpr uint64 mdcr_hyp0 = MDCR_E2TB       |   // Trap trace buffer controls: TRB*
                                            MDCR_E2PB;          // Trap profiling buffer control: PMB*

        static constexpr uint64 mdcr_hyp1 = MDCR_TDCC       |   // Trap debug comms channel: OSDTRRX, OSDTRTX, MDCCSR, MDCCINT, DBGDTR, DBGDTRRX, DBGDTRTX
                                            MDCR_TTRF       |   // Trap trace filter: TRFCR
                                            MDCR_TPMS       |   // Trap performance monitor sampling: PMS*
#if 0
                                            MDCR_TDRA       |   // Trap debug ROM access: MDRAR
                                            MDCR_TDOSA      |   // Trap debug OS register access: DBGPRCR, OSDLR, OSLAR, OSLSR
                                            MDCR_TDA        |   // Trap debug access: DBGBCR*, DBGBVR*, DBGWCR*, DBGWVR*, DBGCLAIM*, DBGAUTHSTATUS, MDCCSR, MDCCINT, MDSCR, OSDTRRX, OSDTRTX, OSECCR
#else
                                            MDCR_TDE        |   // Trap all of TDRA+TDOSA+TDA
#endif
                                            MDCR_TPM;           // Trap performance monitor access: PMCR, PM*

        struct {                                // EL1 Registers
            uint64  afsr0       { 0 };          // Auxiliary Fault Status Register 0
            uint64  afsr1       { 0 };          // Auxiliary Fault Status Register 1
            uint64  amair       { 0 };          // Auxiliary Memory Attribute Indirection Register
            uint64  contextidr  { 0 };          // Context ID Register
            uint64  cpacr       { 0 };          // Architectural Feature Access Control Register
            uint64  elr         { 0 };          // Exception Link Register
            uint64  esr         { 0 };          // Exception Syndrome Register
            uint64  far         { 0 };          // Fault Address Register
            uint64  mair        { 0 };          // Memory Attribute Indirection Register
            uint64  mdscr       { 0 };          // Monitor Debug System Control Register
            uint64  par         { 0 };          // Physical Address Register
            uint64  sctlr       { 0 };          // System Control Register
            uint64  sp          { 0 };          // Stack Pointer
            uint64  spsr        { 0 };          // Saved Program Status Register
            uint64  tcr         { 0 };          // Translation Control Register
            uint64  tpidr       { 0 };          // Software Thread ID Register
            uint64  ttbr0       { 0 };          // Translation Table Base Register 0
            uint64  ttbr1       { 0 };          // Translation Table Base Register 1
            uint64  vbar        { 0 };          // Vector Base Address Register
        } el1;

        struct {                                // EL2 Registers
            uint64  hcr         { 0 };          // Hypervisor Configuration Register
            uint64  hpfar       { 0 };          // Hypervisor IPA Fault Address Register
            uint64  vdisr       { 0 };          // Virtual Deferred Interrupt Status Register
            uint64  vmpidr      { 0 };          // Virtualization Multiprocessor ID Register
            uint64  vpidr       { 0 };          // Virtualization Processor ID Register
        } el2;

        struct {                                // AArch32 Registers
            uint32  dacr        { 0 };          // Domain Access Control Register
            uint32  fpexc       { 0 };          // Floating-Point Exception Control Register
            uint32  hstr        { 0 };          // Hypervisor System Trap Register
            uint32  ifsr        { 0 };          // Instruction Fault Status Register
            uint32  spsr_abt    { 0 };          // Saved Program Status Register (Abort Mode)
            uint32  spsr_fiq    { 0 };          // Saved Program Status Register (FIQ Mode)
            uint32  spsr_irq    { 0 };          // Saved Program Status Register (IRQ Mode)
            uint32  spsr_und    { 0 };          // Saved Program Status Register (Undefined Mode)
        } a32;

        struct {                                // TMR Registers
            uint64  cntvoff     { 0 };          // Virtual Offset Register
            uint64  cntkctl     { 0 };          // Kernel Control Register
            uint64  cntv_cval   { 0 };          // Virtual Timer CompareValue Register
            uint64  cntv_ctl    { 0 };          // Virtual Timer Control Register
        } tmr;

        struct {                                // GIC Registers
            uint64  lr[16]      { 0 };          // List Registers
            uint32  ap0r[4]     { 0 };          // Active Priorities Group 0 Registers
            uint32  ap1r[4]     { 0 };          // Active Priorities Group 1 Registers
            uint32  elrsr       { 0 };          // Empty List Register Status Register
            uint32  vmcr        { 0 };          // Virtual Machine Control Register
            uint32  hcr         { 1 };          // Hypervisor Control Register
        } gic;

        static Vmcb const *current CPULOCAL;

        static void init();
        static void load_hst();

        void load_gst() const;
        void save_gst();

        static inline void *operator new (size_t) noexcept
        {
            return Buddy::alloc (0);
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                Buddy::free (ptr);
        }
};
