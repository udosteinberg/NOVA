/*
 * Architecture Definitions: ARM
 *
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

#include "macros.hpp"

#define ARCH                "aarch64"
#define BFD_ARCH            "aarch64"
#define BFD_FORMAT          "elf64-littleaarch64"
#define ELF_MACHINE         Eh::EM_AARCH64
#define PTE_BPL             9

// Saved Program Status Register
#define SPSR_63_34          BIT64_RANGE (63, 34)    //  -0- -0- -0-
#define SPSR_PPEND          BIT64 (33)              //  EL3 EL2 EL1 PMU Exception Pending
#define SPSR_PM             BIT64 (32)              //  EL3 EL2 EL1 PMU Exception Mask
#define SPSR_NZCV           BIT64_RANGE (31, 28)    //  EL3 EL2 EL1 Condition Flags
#define SPSR_27_26          BIT64_RANGE (27, 26)    //  -0- -0- -0-
#define SPSR_TCO            BIT64 (25)              //  EL3 EL2 EL1 Tag Check Override
#define SPSR_DIT            BIT64 (24)              //  EL3 EL2 EL1 Data Independent Timing
#define SPSR_UAO            BIT64 (23)              //  EL3 EL2 EL1 User Access Override
#define SPSR_PAN            BIT64 (22)              //  EL3 EL2 EL1 Privileged Access Never
#define SPSR_SS             BIT64 (21)              //  EL3 EL2 EL1 Software Step
#define SPSR_IL             BIT64 (20)              //  EL3 EL2 EL1 Illegal Execution State
#define SPSR_19_14          BIT64_RANGE (19, 14)    //  -0- -0- -0-
#define SPSR_ALLINT         BIT64 (13)              //  EL3 EL2 EL1 Mask all IRQ or FIQ
#define SPSR_SSBS           BIT64 (12)              //  EL3 EL2 EL1 Speculative Store Bypass
#define SPSR_BTYPE          BIT64_RANGE (11, 10)    //  EL3 EL2 EL1 Branch Type Indicator
#define SPSR_D              BIT64  (9)              //  EL3 EL2 EL1 Mask Debug Exception
#define SPSR_A              BIT64  (8)              //  EL3 EL2 EL1 Mask SError
#define SPSR_I              BIT64  (7)              //  EL3 EL2 EL1 Mask IRQ
#define SPSR_F              BIT64  (6)              //  EL3 EL2 EL1 Mask FIQ
#define SPSR_5              BIT64  (5)              //  -0- -0- -0-
#define SPSR_M              BIT64_RANGE (4, 0)      //  EL3 EL2 EL1 Execution State
#define SPSR_EL3            VAL64_SHIFT (3, 2)      //  EL3 --- --- AArch64 EL3 (Monitor)
#define SPSR_EL2            VAL64_SHIFT (2, 2)      //  EL3 EL2 --- AArch64 EL2 (Hypervisor)
#define SPSR_EL1            VAL64_SHIFT (1, 2)      //  EL3 EL2 EL1 AArch64 EL1 (Kernel)
#define SPSR_EL0            VAL64_SHIFT (0, 2)      //  EL3 EL2 EL1 AArch64 EL0 (User)
#define SPSR_SP             BIT64  (0)              //  EL3 EL2 EL1 Stack Pointer Select

// System Control Register
#define SCTLR_TIDCP         BIT64 (63)              //  -0- EL2 EL1 Trap Implementation Defined
#define SCTLR_SPINTMASK     BIT64 (62)              //  EL3 EL2 EL1 Superpriority Interrupt Mask Enable
#define SCTLR_NMI           BIT64 (61)              //  EL3 EL2 EL1 Non-Maskable Interrupt Enable
#define SCTLR_EnTP2         BIT64 (60)              //  -0- EL2 EL1 Enable EL0 TPIDR2
#define SCTLR_TCSO          BIT64 (59)              //  EL3 EL2 EL1 Tag Checking Store Only
#define SCTLR_TCSO0         BIT64 (58)              //  -0- EL2 EL1 Checking Store Only in EL0
#define SCTLR_EPAN          BIT64 (57)              //  -0- EL2 EL1 Enhanced Privilege Access Never
#define SCTLR_EnALS         BIT64 (56)              //  -0- EL2 EL1 Enable EL0 LD64B/ST64B
#define SCTLR_EnAS0         BIT64 (55)              //  -0- EL2 EL1 Enable EL0 ST64BV0
#define SCTLR_EnASR         BIT64 (54)              //  -0- EL2 EL1 Enable EL0 ST64BV
#define SCTLR_TME           BIT64 (53)              //  EL3 EL2 EL1 Transactional Memory Enable ELx
#define SCTLR_TME0          BIT64 (52)              //  -0- EL2 EL1 Transactional Memory Enable EL0
#define SCTLR_TMT           BIT64 (51)              //  EL3 EL2 EL1 Transactional Memory Trivial ELx
#define SCTLR_TMT0          BIT64 (50)              //  -0- EL2 EL1 Transactional Memory Trivial EL0
#define SCTLR_TWEDEL        BIT64_RANGE (49, 46)    //  -0- EL2 EL1 TWE Delay
#define SCTLR_TWEDEn        BIT64 (45)              //  -0- EL2 EL1 TWE Delay Enable
#define SCTLR_DSSBS         BIT64 (44)              //  EL3 EL2 EL1 Default PSTATE.SSBS on Exception Entry
#define SCTLR_ATA           BIT64 (43)              //  EL3 EL2 EL1 Allocation Tag Access in ELx
#define SCTLR_ATA0          BIT64 (42)              //  -0- EL2 EL1 Allocation Tag Access in EL0
#define SCTLR_TCF           BIT64_RANGE (41, 40)    //  EL3 EL2 EL1 Tag Check Fault in ELx
#define SCTLR_TCF0          BIT64_RANGE (39, 38)    //  -0- EL2 EL1 Tag Check Fault in EL0
#define SCTLR_ITFSB         BIT64 (37)              //  EL3 EL2 EL1 Tag Check Fault Synchronization
#define SCTLR_BT            BIT64 (36)              //  EL3 EL2 EL1 PAC Branch Type Compatibility in ELx
#define SCTLR_BT0           BIT64 (35)              //  -0- EL2 EL1 PAC Branch Type Compatibility in EL0
#define SCTLR_34            BIT64 (34)              //  -0- -0- -0-
#define SCTLR_MSCEn         BIT64 (33)              //  -0- EL2 EL1 Memory Set/Copy Instructions Enable
#define SCTLR_CMOW          BIT64 (32)              //  -0- EL2 EL1 Cache Maintenance Instruction Faults
#define SCTLR_EnIA          BIT64 (31)              //  EL3 EL2 EL1 Enable Pointer Authentication
#define SCTLR_EnIB          BIT64 (30)              //  EL3 EL2 EL1 Enable Pointer Authentication
#define SCTLR_LSMAOE        BIT64 (29)              //  -1- EL2 EL1 Load/Store Multiple Atomicity/Ordering
#define SCTLR_nTLSMD        BIT64 (28)              //  -1- EL2 EL1 Don't trap Load/Store Multiple
#define SCTLR_EnDA          BIT64 (27)              //  EL3 EL2 EL1 Enable Pointer Authentication
#define SCTLR_UCI           BIT64 (26)              //  -0- EL2 EL1 Enable EL0 DC CVAU/CVAC/CVAP/CVADP/CIVAC, IC IVAU
#define SCTLR_EE            BIT64 (25)              //  EL3 EL2 EL1 Endianness of Data Accesses ELx
#define SCTLR_EOE           BIT64 (24)              //  -0- EL2 EL1 Endianness of Data Accesses EL0
#define SCTLR_SPAN          BIT64 (23)              //  -1- EL2 EL1 Set Privilege Access Never
#define SCTLR_EIS           BIT64 (22)              //  EL3 EL2 EL1 Exception Entry is Context-Synchronizing
#define SCTLR_IESB          BIT64 (21)              //  EL3 EL2 EL1 Implicit Error Synchronization
#define SCTLR_TSCXT         BIT64 (20)              //  -0- EL2 EL1 Trap EL0 SXCTNUM
#define SCTLR_WXN           BIT64 (19)              //  EL3 EL2 EL1 Write implies XN
#define SCTLR_nTWE          BIT64 (18)              //  -1- EL2 EL1 Don't trap WFE
#define SCTLR_17            BIT64 (17)              //  -0- -0- -0-
#define SCTLR_nTWI          BIT64 (16)              //  -1- EL2 EL1 Don't trap WFI
#define SCTLR_UCT           BIT64 (15)              //  -0- EL2 EL1 Enable EL0 CTR_EL0
#define SCTLR_DZE           BIT64 (14)              //  -0- EL2 EL1 Enable EL0 DC ZVA
#define SCTLR_EnDB          BIT64 (13)              //  EL3 EL2 EL1 Enable Pointer Authentication
#define SCTLR_I             BIT64 (12)              //  EL3 EL2 EL1 Enable I-Cache
#define SCTLR_EOS           BIT64 (11)              //  EL3 EL2 EL1 Exception Exit is Context-Synchronizing
#define SCTLR_EnRCTX        BIT64 (10)              //  -0- EL2 EL1 Enable EL0 RCTX instructions
#define SCTLR_UMA           BIT64  (9)              //  -0- -0- EL1 User Mask Access
#define SCTLR_SED           BIT64  (8)              //  -0- EL2 EL1 Disable EL0 SETEND (AArch32)
#define SCTLR_ITD           BIT64  (7)              //  -0- EL2 EL1 Disable EL0 IT instructions (AArch32)
#define SCTLR_nAA           BIT64  (6)              //  EL3 EL2 EL1 Enable Non-Aligned Access
#define SCTLR_CP15BEN       BIT64  (5)              //  -1- EL2 EL1 Enable EL0 DMB, DSB, ISB (AArch32)
#define SCTLR_SA0           BIT64  (4)              //  -1- EL2 EL1 Enable EL0 Stack Alignment Check
#define SCTLR_SA            BIT64  (3)              //  EL3 EL2 EL1 Enable ELx Stack Alignment Check
#define SCTLR_C             BIT64  (2)              //  EL3 EL2 EL1 Enable D-Cache
#define SCTLR_A             BIT64  (1)              //  EL3 EL2 EL1 Enable Alignment Check
#define SCTLR_M             BIT64  (0)              //  EL3 EL2 EL1 Enable Stage-1 MMU

// Translation Control Register
#define TCR_63_44           BIT64_RANGE (63, 44)    //  -0- -0- ---
#define TCR_DisCH0          BIT64 (43)              //  EL3 -0- --- Disable Contiguous Hint for Start Table
#define TCR_HAFT            BIT64 (42)              //  EL3 -0- --- Hardware-Managed Access Flag for Tables
#define TCR_PTTWI           BIT64 (41)              //  EL3 -0- --- Permit Translation Table Walk Incoherence
#define TCR_40_39           BIT64_RANGE (40, 39)    //  -0- -0- ---
#define TCR_D128            BIT64 (38)              //  EL3 -0- --- Enable VMSA-128
#define TCR_AIE             BIT64 (37)              //  EL3 -0- --- Attribute Indexing Enable
#define TCR_POE             BIT64 (36)              //  EL3 -0- --- Permission Overlay Enable
#define TCR_PIE             BIT64 (35)              //  EL3 -0- --- Permission Indirection Enable
#define TCR_PnCH            BIT64 (34)              //  EL3 -0- --- Protected Attribute Enable
#define TCR_MTX             BIT64 (33)              //  EL3 EL2 --- Extended Memory Tag Checking
#define TCR_DS              BIT64 (32)              //  EL3 EL2 --- Translation Descriptor Size
#define TCR_31              BIT64 (31)              //  -1- -1- ---
#define TCR_TCMA            BIT64 (30)              //  EL3 EL2 --- Unchecked Access at ELx
#define TCR_TBID            BIT64 (29)              //  EL3 EL2 --- Top Byte Instruction/Data Matching
#define TCR_HWU62           BIT64 (28)              //  EL3 EL2 --- Hardware Use Bit62
#define TCR_HWU61           BIT64 (27)              //  EL3 EL2 --- Hardware Use Bit61
#define TCR_HWU60           BIT64 (26)              //  EL3 EL2 --- Hardware Use Bit60
#define TCR_HWU59           BIT64 (25)              //  EL3 EL2 --- Hardware Use Bit59
#define TCR_HPD             BIT64 (24)              //  EL3 EL2 --- Hierarchical Permission Disable
#define TCR_23              BIT64 (23)              //  -1- -1- ---
#define TCR_HD              BIT64 (22)              //  EL3 EL2 --- Hardware-Managed Dirty Flag
#define TCR_HA              BIT64 (21)              //  EL3 EL2 --- Hardware-Managed Access Flag
#define TCR_TBI             BIT64 (20)              //  EL3 EL2 --- Top Byte Ignored
#define TCR_19              BIT64 (19)              //  -0- -0-
#define TCR_PS              BIT64_RANGE (18, 16)    //  EL3 EL2 --- Physical Address Size
#define TCR_TG0_16K         VAL64_SHIFT (2, 14)     //  EL3 EL2 --- TTBR0 Granule 16K
#define TCR_TG0_64K         VAL64_SHIFT (1, 14)     //  EL3 EL2 --- TTBR0 Granule 64K
#define TCR_TG0_4K          VAL64_SHIFT (0, 14)     //  EL3 EL2 --- TTBR0 Granule 4K
#define TCR_SH0_INNER       VAL64_SHIFT (3, 12)     //  EL3 EL2 --- TTBR0 Inner Shareable
#define TCR_SH0_OUTER       VAL64_SHIFT (2, 12)     //  EL3 EL2 --- TTBR0 Outer Shareable
#define TCR_SH0_NONE        VAL64_SHIFT (0, 12)     //  EL3 EL2 --- TTBR0 Non-Shareable
#define TCR_ORGN0_WB_R      VAL64_SHIFT (3, 10)     //  EL3 EL2 --- TTBR0 Normal Memory, Outer WB, Read-Allocate
#define TCR_ORGN0_WT_R      VAL64_SHIFT (2, 10)     //  EL3 EL2 --- TTBR0 Normal Memory, Outer WT, Read-Allocate
#define TCR_ORGN0_WB_RW     VAL64_SHIFT (1, 10)     //  EL3 EL2 --- TTBR0 Normal Memory, Outer WB, Read-Allocate, Write-Allocate
#define TCR_ORGN0_NC        VAL64_SHIFT (0, 10)     //  EL3 EL2 --- TTBR0 Normal Memory, Outer NC
#define TCR_IRGN0_WB_R      VAL64_SHIFT (3,  8)     //  EL3 EL2 --- TTBR0 Normal Memory, Inner WB, Read-Allocate
#define TCR_IRGN0_WT_R      VAL64_SHIFT (2,  8)     //  EL3 EL2 --- TTBR0 Normal Memory, Inner WT, Read-Allocate
#define TCR_IRGN0_WB_RW     VAL64_SHIFT (1,  8)     //  EL3 EL2 --- TTBR0 Normal Memory, Inner WB, Read-Allocate, Write-Allocate
#define TCR_IRGN0_NC        VAL64_SHIFT (0,  8)     //  EL3 EL2 --- TTBR0 Normal Memory, Inner NC
#define TCR_7_6             BIT64_RANGE (7,  6)     //  -0- -0- ---
#define TCR_T0SZ(X)         (64 - (X))              //  EL3 EL2 --- TTBR0 Translation Size

// Virtualization Translation Control Register (EL2)
#define VTCR_63_45          BIT64_RANGE (63, 45)
#define VTCR_HAFT           BIT64 (44)              // Hardware-Managed Access Flag for Tables
#define VTCR_43_42          BIT64_RANGE (43, 42)
#define VTCR_TL0            BIT64 (41)              // MMU TopLevel0 Permission Attribute
#define VTCR_40_39          BIT64_RANGE (40, 39)
#define VTCR_D128           BIT64 (38)              // Enable 128-bit Page Table Descriptors
#define VTCR_S2POE          BIT64 (37)              // Enable Stage-2 Permission Overlay
#define VTCR_S2PIE          BIT64 (36)              // Select Stage-2 Permission Model
#define VTCR_TL1            BIT64 (35)              // MMU TopLevel1 Permission Attribute
#define VTCR_ASSUREDONLY    BIT64 (34)              // Enable AssuredOnly Attribute
#define VTCR_SL2            BIT64 (33)              // Stage-2 Starting Level
#define VTCR_DS             BIT64 (32)              // Stage-2 Descriptor Size
#define VTCR_31             BIT64 (31)
#define VTCR_NSA            BIT64 (30)              // Non-Secure Stage-2 Output Address Space
#define VTCR_NSW            BIT64 (29)              // Non-Secure Stage-2 Translation Table Space
#define VTCR_HWU62          BIT64 (28)              // Hardware Use
#define VTCR_HWU61          BIT64 (27)              // Hardware Use
#define VTCR_HWU60          BIT64 (26)              // Hardware Use
#define VTCR_HWU59          BIT64 (25)              // Hardware Use
#define VTCR_24_23          BIT64_RANGE (24, 23)
#define VTCR_HD             BIT64 (22)              // Hardware Update D-Flag
#define VTCR_HA             BIT64 (21)              // Hardware Update A-Flag
#define VTCR_20             BIT64 (20)
#define VTCR_VS             BIT64 (19)              // VMID Size
#define VTCR_RES0           (VTCR_63_45 | VTCR_43_42 | VTCR_40_39 | VTCR_24_23 | VTCR_20)
#define VTCR_RES1           (VTCR_31)

// Architectural Feature Trap Register (EL2)
#define CPTR_63_32          BIT64_RANGE (63, 32)
#define CPTR_TCPAC          BIT64 (31)              // Trap CPACR
#define CPTR_TAM            BIT64 (30)              // Trap Activity Monitor Access
#define CPTR_29_21          BIT64_RANGE (29, 21)
#define CPTR_TTA            BIT64 (20)              // Trap Trace Register Access
#define CPTR_19_14          BIT64_RANGE (19, 14)
#define CPTR_13             BIT64 (13)
#define CPTR_TSM            BIT64 (12)              // Trap SME instructions
#define CPTR_11             BIT64 (11)
#define CPTR_TFP            BIT64 (10)              // Trap SVE/SIMD/FP Functionality
#define CPTR_9              BIT64  (9)
#define CPTR_TZ             BIT64  (8)              // Trap SVE Functionality
#define CPTR_7_0            BIT64_RANGE (7, 0)
#define CPTR_RES0           (CPTR_63_32 | CPTR_29_21 | CPTR_19_14 | CPTR_11)
#define CPTR_RES1           (CPTR_13 | CPTR_9 | CPTR_7_0)

// Monitor Debug Configuration Register (EL2)
#define MDCR_63_44          BIT64_RANGE (63, 44)
#define MDCR_EBWE           BIT64 (43)              // Extended Breakpoint and Watchpoint Enable
#define MDCR_42             BIT64 (42)
#define MDCR_PMEE           BIT64_RANGE (41, 40)    // Performance Monitors Exception Enable
#define MDCR_39_37          BIT64_RANGE (39, 37)
#define MDCR_HPMFZS         BIT64 (36)              // Hypervisor PerfMon Freeze on SPE Event
#define MDCR_35_30          BIT64_RANGE (35, 30)
#define MDCR_HPMFZO         BIT64 (29)              // Hypervisor PerfMon Freeze on Overflow
#define MDCR_MTPME          BIT64 (28)              // Multithreaded PMU Enable
#define MDCR_TDCC           BIT64 (27)              // Trap Debug Comms Channel
#define MDCR_HLP            BIT64 (26)              // Hypervisor Long Event Counter Enable
#define MDCR_E2TB           BIT64_RANGE (25, 24)    // EL2 Trace Buffer
#define MDCR_HCCD           BIT64 (23)              // Hypervisor Cycle Counter Disable
#define MDCR_22_20          BIT64_RANGE (22, 20)
#define MDCR_TTRF           BIT64 (19)              // Trap Trace Filter Control
#define MDCR_18             BIT64 (18)
#define MDCR_HPMD           BIT64 (17)              // Guest Performance Monitor Disable
#define MDCR_16_15          BIT64_RANGE (16, 15)
#define MDCR_TPMS           BIT64 (14)              // Trap Performance Monitor Sampling
#define MDCR_E2PB           BIT64_RANGE (13, 12)    // EL2 Profiling Buffer
#define MDCR_TDRA           BIT64 (11)              // Trap Debug ROM Address Register Access
#define MDCR_TDOSA          BIT64 (10)              // Trap Debug OS-Related Register Access
#define MDCR_TDA            BIT64  (9)              // Trap Debug Access
#define MDCR_TDE            BIT64  (8)              // Trap Debug Exceptions
#define MDCR_HPME           BIT64  (7)              // Event Counters Enable
#define MDCR_TPM            BIT64  (6)              // Trap Performance Monitor Access
#define MDCR_TPMCR          BIT64  (5)              // Trap PMCR Access
#define MDCR_HPMN           BIT64_RANGE (4, 0)      // Accessible Event Counters
#define MDCR_RES0           (MDCR_63_44 | MDCR_42 | MDCR_39_37 | MDCR_35_30 | MDCR_22_20 | MDCR_18 | MDCR_16_15)
#define MDCR_RES1           (SFX_ULL(0))

// Hypervisor Configuration Register (EL2)
#define HCR_TWEDEL          BIT64_RANGE (63, 60)    // TWE Delay
#define HCR_TWEDEn          BIT64 (59)              // TWE Delay Enable
#define HCR_TID5            BIT64 (58)              // Trap ID Group 5
#define HCR_DCT             BIT64 (57)              // Default Cacheability Tagging
#define HCR_ATA             BIT64 (56)              // Allocation Tag Access
#define HCR_TTLBOS          BIT64 (55)              // Trap TLB Maintenance Outer Shareable
#define HCR_TTLBIS          BIT64 (54)              // Trap TLB Maintenance Inner Shareable
#define HCR_ENSCXT          BIT64 (53)              // Enable Access to SCXTNUM
#define HCR_TOCU            BIT64 (52)              // Trap PoU Cache Instructions
#define HCR_AMVOFFEN        BIT64 (51)              // Activity Monitor Virtual Offset Enable
#define HCR_TICAB           BIT64 (50)              // Trap IC IALLUIS
#define HCR_TID4            BIT64 (49)              // Trap ID Group 4
#define HCR_GPF             BIT64 (48)              // Trap Granule Protection Faults
#define HCR_FIEN            BIT64 (47)              // Fault Injection Enable
#define HCR_FWB             BIT64 (46)              // Combined Cacheability
#define HCR_NV2             BIT64 (45)              // Nested Virtualization
#define HCR_AT              BIT64 (44)              // Trap Address Translation
#define HCR_NV1             BIT64 (43)              // Nested Virtualization
#define HCR_NV              BIT64 (42)              // Nested Virtualization
#define HCR_API             BIT64 (41)              // Trap Pointer Authentication Instructions
#define HCR_APK             BIT64 (40)              // Trap Key Values
#define HCR_TME             BIT64 (39)              // Transactional Memory Enable
#define HCR_MIOCNCE         BIT64 (38)              // Trap Mismatched Inner/Outer Cacheable Non-Coherency
#define HCR_TEA             BIT64 (37)              // Trap External Aborts
#define HCR_TERR            BIT64 (36)              // Trap Error Record Accesses
#define HCR_TLOR            BIT64 (35)              // Trap LOR Registers
#define HCR_E2H             BIT64 (34)              // Enable EL2 Host
#define HCR_ID              BIT64 (33)              // Stage-2 Inst Cache Disable
#define HCR_CD              BIT64 (32)              // Stage-2 Data Cache Disable
#define HCR_RW              BIT64 (31)              // Register Width for Lower ELs
#define HCR_TRVM            BIT64 (30)              // Trap Reads of Virtual Memory Controls
#define HCR_HCD             BIT64 (29)              // Disable HVC
#define HCR_TDZ             BIT64 (28)              // Trap DC ZVA
#define HCR_TGE             BIT64 (27)              // Trap General Exceptions
#define HCR_TVM             BIT64 (26)              // Trap Writes of Virtual Memory Controls
#define HCR_TTLB            BIT64 (25)              // Trap TLB Maintenance
#define HCR_TPU             BIT64 (24)              // Trap Cache Maintenance to PoU
#define HCR_TPCP            BIT64 (23)              // Trap Cache Maintenance to PoC or PoP
#define HCR_TSW             BIT64 (22)              // Trap Cache Maintenance by Set/Way
#define HCR_TACR            BIT64 (21)              // Trap Auxiliary Control Registers
#define HCR_TIDCP           BIT64 (20)              // Trap Implementation-Defined
#define HCR_TSC             BIT64 (19)              // Trap SMC
#define HCR_TID3            BIT64 (18)              // Trap ID Group 3
#define HCR_TID2            BIT64 (17)              // Trap ID Group 2
#define HCR_TID1            BIT64 (16)              // Trap ID Group 1
#define HCR_TID0            BIT64 (15)              // Trap ID Group 0
#define HCR_TWE             BIT64 (14)              // Trap WFE from EL0/EL1
#define HCR_TWI             BIT64 (13)              // Trap WFI from EL0/EL1
#define HCR_DC              BIT64 (12)              // Default Cacheability
#define HCR_BSU_SYSTEM      VAL64_SHIFT (3, 10)     // Barrier Shareability Upgrade
#define HCR_BSU_OUTER       VAL64_SHIFT (2, 10)     // Barrier Shareability Upgrade
#define HCR_BSU_INNER       VAL64_SHIFT (1, 10)     // Barrier Shareability Upgrade
#define HCR_BSU_NONE        VAL64_SHIFT (0, 10)     // Barrier Shareability Upgrade
#define HCR_FB              BIT64  (9)              // Force Broadcast
#define HCR_VSE             BIT64  (8)              // Virtual Asynchronous Abort Exception
#define HCR_VI              BIT64  (7)              // Virtual IRQ Exception
#define HCR_VF              BIT64  (6)              // Virtual FIQ Exception
#define HCR_AMO             BIT64  (5)              // Asynchronous Abort Mask Override
#define HCR_IMO             BIT64  (4)              // IRQ Mask Override
#define HCR_FMO             BIT64  (3)              // FIQ Mask Override
#define HCR_PTW             BIT64  (2)              // Protected Table Walk
#define HCR_SWIO            BIT64  (1)              // Set/Way Invalidation Override
#define HCR_VM              BIT64  (0)              // Enable Stage-2 MMU

// Extended Hypervisor Configuration Register (EL2)
#define HCRX_63_22          BIT64_RANGE (63, 22)
#define HCRX_EnIDCP128      BIT64 (21)              // Enable Implementation-Defined 128-bit System registers
#define HCRX_EnSDERR        BIT64 (20)              // Enable Synchronous Device Read Error
#define HCRX_19             BIT64 (19)
#define HCRX_EnSNERR        BIT64 (18)              // Enable Synchronous Normal Read Error
#define HCRX_D128En         BIT64 (17)              // 128-bit System Register Trap Control
#define HCRX_PTTWI          BIT64 (16)              // Permit Translation Table Walk Incoherence
#define HCRX_SCTLR2En       BIT64 (15)              // SCTLR2_EL1 Enable
#define HCRX_TCR2En         BIT64 (14)              // TCR2_EL1 Enable
#define HCRX_13_12          BIT64_RANGE (13, 12)
#define HCRX_MSCEn          BIT64 (11)              // Memory Set/Copy Instructions Enable
#define HCRX_MCE2           BIT64 (10)              // Trap Memory Set/Copy Exceptions
#define HCRX_CMOW           BIT64  (9)              // Cache Maintenance Instruction Faults
#define HCRX_VFNMI          BIT64  (8)              // Virtual FIQ with Superpriority
#define HCRX_VINMI          BIT64  (7)              // Virtual IRQ with Superpriority
#define HCRX_TALLINT        BIT64  (6)              // Trap ALLINT MSR writes
#define HCRX_SMPME          BIT64  (5)              // Streaming Mode Priority Mapping Enable
#define HCRX_FGTnXS         BIT64  (4)              // Fine-Grained Trap of TLBI nXS
#define HCRX_FnXS           BIT64  (3)              // TLBI nXS Promotion
#define HCRX_EnASR          BIT64  (2)              // Enable EL0 ST64BV
#define HCRX_EnALS          BIT64  (1)              // Enable EL0 LD64B/ST64B
#define HCRX_EnAS0          BIT64  (0)              // Enable EL0 ST64BV0
#define HCRX_RES0           (HCRX_63_22 | HCRX_19 | HCRX_13_12)

// Secure Configuration Register (EL3)
#define SCR_63              BIT64 (63)
#define SCR_NSE             BIT64 (62)              // Security State of EL2
#define SCR_61_60           BIT64_RANGE (61, 60)
#define SCR_FGTEn2          BIT64 (59)              // Fine-Grained Traps Enable 2
#define SCR_58_56           BIT64_RANGE (58, 56)
#define SCR_EnIDCP128       BIT64 (55)              // Enable Implementation-Defined 128-bit System registers
#define SCR_54              BIT64 (54)
#define SCR_PFAREn          BIT64 (53)              // Enable Physical Fault Address Registers
#define SCR_TWERR           BIT64 (52)              // Trap Writes of Error Record Registers
#define SCR_51              BIT64 (51)
#define SCR_ERR2En          BIT64 (50)              // Enable Additional Error Record Registers
#define SCR_MECEn           BIT64 (49)              // Enable EL2 MECID Registers
#define SCR_GPF             BIT64 (48)              // Trap Granule Protection Faults
#define SCR_D128En          BIT64 (47)              // 128-bit System Register Trap Control
#define SCR_AIEn            BIT64 (46)              // MAIR2_ELx, AMAIR2_ELx Register Access Trap Control
#define SCR_PIEn            BIT64 (45)              // Permission Indirection Overlay Register Access Trap Control
#define SCR_SCTLR2En        BIT64 (44)              // SCTLR2_ELx Register Trap Control
#define SCR_TCR2En          BIT64 (43)              // TCR2_ELx Register Trap Control
#define SCR_RCWMASKEn       BIT64 (42)              // RCW and RCWS Mask Register Trap Control
#define SCR_EnTP2           BIT64 (41)              // Trap TPIDR2_EL0
#define SCR_TRNDR           BIT64 (40)              // Trap RNDR and RNDRRS Reads
#define SCR_EnCSR           BIT64 (39)              // Call Stack Recorder Enable
#define SCR_HXEn            BIT64 (38)              // HCRX_EL2 Enable
#define SCR_ADEn            BIT64 (37)              // ACCDATA_EL1 Enable
#define SCR_EnAS0           BIT64 (36)              // Trap ST64BV0 from EL0/EL1/EL2
#define SCR_AMVOFFEN        BIT64 (35)              // Activity Monitor Virtual Offset Enable
#define SCR_TME             BIT64 (34)              // Transactional Memory Enable
#define SCR_TWEDEL          BIT64_RANGE (33, 30)    // TWE Delay
#define SCR_TWEDEn          BIT64 (29)              // TWE Delay Enable
#define SCR_ECVEn           BIT64 (28)              // ECV Enable
#define SCR_FGTEn           BIT64 (27)              // FGT Enable
#define SCR_ATA             BIT64 (26)              // Allocation Tag Access
#define SCR_ENSCXT          BIT64 (25)              // Enable Access to SCXTNUM
#define SCR_24_22           BIT64_RANGE (24, 22)
#define SCR_FIEN            BIT64 (21)              // Fault Injection Enable
#define SCR_NMEA            BIT64 (20)              // Non-Maskable External Aborts
#define SCR_EASE            BIT64 (19)              // External Aborts to SError
#define SCR_EEL2            BIT64 (18)              // Enable Secure EL2
#define SCR_API             BIT64 (17)              // Trap Pointer Authentication Instructions
#define SCR_APK             BIT64 (16)              // Trap Key Values
#define SCR_TERR            BIT64 (15)              // Trap Error record Accesses
#define SCR_TLOR            BIT64 (14)              // Trap LOR Registers
#define SCR_TWE             BIT64 (13)              // Trap WFE from EL0/EL1/EL2
#define SCR_TWI             BIT64 (12)              // Trap WFI from EL0/EL1/EL2
#define SCR_ST              BIT64 (11)              // Trap Secure Timer
#define SCR_RW              BIT64 (10)              // Execution State Lower EL
#define SCR_SIF             BIT64  (9)              // Secure Instruction Fetch
#define SCR_HCE             BIT64  (8)              // Enable HVC
#define SCR_SMD             BIT64  (7)              // Disable SMC
#define SCR_6               BIT64  (6)
#define SCR_5_4             BIT64_RANGE (5, 4)
#define SCR_EA              BIT64  (3)              // External Abort and SError Routing
#define SCR_FIQ             BIT64  (2)              // Physical FIQ Routing
#define SCR_IRQ             BIT64  (1)              // Physical IRQ Routing
#define SCR_NS              BIT64  (0)              // Non-Secure
#define SCR_RES0            (SCR_63 | SCR_61_60 | SCR_58_56 | SCR_54 | SCR_51 | SCR_24_22 | SCR_6)
#define SCR_RES1            (SCR_5_4)
