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
#define IPA_BITS            39

#define SPSR_A32_63_32      BIT64_RANGE (63, 32)
#define SPSR_A64_63_33      BIT64_RANGE (63, 33)
#define SPSR_A64_CSR        BIT64 (32)              // Call Stack Recording
#define SPSR_ALL_N          BIT64 (31)              // Condition Negative
#define SPSR_ALL_Z          BIT64 (30)              // Condition Zero
#define SPSR_ALL_C          BIT64 (29)              // Condition Carry
#define SPSR_ALL_V          BIT64 (28)              // Condition Overflow
#define SPSR_A32_Q          BIT64 (27)              // Saturation
#define SPSR_A64_27_26      BIT64_RANGE (27, 26)
#define SPSR_A32_IT_1_0     BIT64_RANGE (26, 25)    // If-Then
#define SPSR_A64_TCO        BIT64 (25)              // Tag Check Override
#define SPSR_ALL_DIT        BIT64 (24)              // Data Independent Timing
#define SPSR_A32_SSBS       BIT64 (23)              // Speculative Store Bypass
#define SPSR_A64_UAO        BIT64 (23)              // User Access Override
#define SPSR_ALL_PAN        BIT64 (22)              // Privileged Access Never
#define SPSR_ALL_SS         BIT64 (21)              // Software Step
#define SPSR_ALL_IL         BIT64 (20)              // Illegal Execution State
#define SPSR_A32_GE         BIT64_RANGE (19, 16)    // Greater-Than or Equal
#define SPSR_A64_19_14      BIT64_RANGE (19, 14)
#define SPSR_A32_IT_7_2     BIT64_RANGE (15, 10)    // If-Then
#define SPSR_A64_ALLINT     BIT64 (13)              // Mask all IRQ or FIQ
#define SPSR_A64_SSBS       BIT64 (12)              // Speculative Store Bypass
#define SPSR_A64_BTYPE      BIT64_RANGE (11, 10)    // Branch Type Indicator
#define SPSR_A32_E          BIT64  (9)              // Endianness
#define SPSR_A64_D          BIT64  (9)              // Mask Debug Exception
#define SPSR_ALL_A          BIT64  (8)              // Mask SError
#define SPSR_ALL_I          BIT64  (7)              // Mask IRQ
#define SPSR_ALL_F          BIT64  (6)              // Mask FIQ
#define SPSR_A32_T          BIT64  (5)              // T32 State
#define SPSR_A64_5          BIT64  (5)
#define SPSR_ALL_nRW        BIT64  (4)              // Execution State (0=AArch64, 1=AArch32)
#define SPSR_ALL_M          BIT64_RANGE (3, 0)      // Mode
#define SPSR_A64_EL3        VAL64_SHIFT (3, 2)      // EL3 (Monitor)
#define SPSR_A64_EL2        VAL64_SHIFT (2, 2)      // EL2 (Hypervisor)
#define SPSR_A64_EL1        VAL64_SHIFT (1, 2)      // EL1 (Kernel)
#define SPSR_A64_EL0        VAL64_SHIFT (0, 2)      // EL0 (User)
#define SPSR_A64_SP         BIT64  (0)              // Stack Pointer Select
#define SPSR_A32_RES0       (SPSR_A32_63_32)
#define SPSR_A64_RES0       (SPSR_A64_63_33 | SPSR_A64_27_26 | SPSR_A64_19_14 | SPSR_A64_5)

#define SCTLR_A32_63_32     BIT64_RANGE (63, 32)
#define SCTLR_A64_TIDCP     BIT64 (63)              // Trap Implementation Defined
#define SCTLR_A64_SPINTMASK BIT64 (62)              // Superpriority Interrupt Mask Enable
#define SCTLR_A64_NMI       BIT64 (61)              // Non-Maskable Interrupt Enable
#define SCTLR_A64_EnTP2     BIT64 (60)              // Enable TPIDR2 in EL0
#define SCTLR_A64_EnCSR0    BIT64_RANGE (59, 58)    // EL0 Call Stack Recorder Enable
#define SCTLR_A64_EPAN      BIT64 (57)              // Enhanced Privilege Access Never
#define SCTLR_A64_EnALS     BIT64 (56)              // Enable EL0 LD64B/ST64B
#define SCTLR_A64_EnAS0     BIT64 (55)              // Enable EL0 ST64BV0
#define SCTLR_A64_EnASR     BIT64 (54)              // Enable EL0 ST64BV
#define SCTLR_A64_TME       BIT64 (53)              // Transactional Memory Enable EL1/EL2/EL3
#define SCTLR_A64_TME0      BIT64 (52)              // Transactional Memory Enable EL0
#define SCTLR_A64_TMT       BIT64 (51)              // Transactional Memory Trivial EL1/EL2/EL3
#define SCTLR_A64_TMT0      BIT64 (50)              // Transactional Memory Trivial EL0
#define SCTLR_A64_TWEDEL    BIT64_RANGE (49, 46)    // TWE Delay
#define SCTLR_A64_TWEDEn    BIT64 (45)              // TWE Delay Enable
#define SCTLR_A64_DSSBS     BIT64 (44)              // Default PSTATE.SSBS on Exception Entry
#define SCTLR_A64_ATA       BIT64 (43)              // Allocation Tag Access in EL1/EL2
#define SCTLR_A64_ATA0      BIT64 (42)              // Allocation Tag Access in EL0
#define SCTLR_A64_TCF       BIT64_RANGE (41, 40)    // Tag Check Fault in EL1/EL2
#define SCTLR_A64_TCF0      BIT64_RANGE (39, 38)    // Tag Check Fault in EL0
#define SCTLR_A64_ITFSB     BIT64 (37)              // Tag Check Fault Synchronization
#define SCTLR_A64_BT1       BIT64 (36)              // PAC Branch Type Compatibility in EL1/EL2
#define SCTLR_A64_BT0       BIT64 (35)              // PAC Branch Type Compatibility in EL0
#define SCTLR_A64_34        BIT64 (34)
#define SCTLR_A64_MSCEn     BIT64 (33)              // Memory Set/Copy Instructions Enable
#define SCTLR_A64_CMOW      BIT64 (32)              // Cache Maintenance Instruction Faults
#define SCTLR_A32_DSSBS     BIT64 (31)              // Default SSBS on Exception Entry
#define SCTLR_A64_EnIA      BIT64 (31)              // Enable Pointer Authentication
#define SCTLR_A32_TE        BIT64 (30)              // T32 Exception Enable
#define SCTLR_A64_EnIB      BIT64 (30)              // Enable Pointer Authentication
#define SCTLR_A32_AFE       BIT64 (29)              // Access Flag Enable
#define SCTLR_A64_LSMAOE    BIT64 (29)              // Load/Store Multiple Atomicity/Ordering
#define SCTLR_A32_TRE       BIT64 (28)              // TEX Remap Enable
#define SCTLR_A64_nTLSMD    BIT64 (28)              // Don't trap Load/Store Multiple
#define SCTLR_A32_27_26     BIT64_RANGE (27, 26)
#define SCTLR_A64_EnDA      BIT64 (27)              // Enable Pointer Authentication
#define SCTLR_A64_UCI       BIT64 (26)              // Enable EL0 DC CVAU/CVAC/CVAP/CVADP/CIVAC, IC IVAU
#define SCTLR_ALL_EE        BIT64 (25)              // Endianness of Data Accesses EL1/EL2
#define SCTLR_A32_24        BIT64 (24)
#define SCTLR_A64_EOE       BIT64 (24)              // Endianness of Data Accesses EL0
#define SCTLR_ALL_SPAN      BIT64 (23)              // Set Privilege Access Never
#define SCTLR_A32_22        BIT64 (22)
#define SCTLR_A64_EIS       BIT64 (22)              // Exception Entry is Context-Synchronizing
#define SCTLR_A32_21        BIT64 (21)
#define SCTLR_A64_IESB      BIT64 (21)              // Implicit Error Synchronization
#define SCTLR_A32_UWXN      BIT64 (20)              // Unprivileged write implies XN
#define SCTLR_A64_TSCXT     BIT64 (20)              // Trap EL0 SXCTNUM
#define SCTLR_ALL_WXN       BIT64 (19)              // Write implies XN
#define SCTLR_ALL_nTWE      BIT64 (18)              // Don't trap WFE
#define SCTLR_ALL_17        BIT64 (17)
#define SCTLR_ALL_nTWI      BIT64 (16)              // Don't trap WFI
#define SCTLR_A32_15_14     BIT64_RANGE (15, 14)
#define SCTLR_A64_UCT       BIT64 (15)              // Enable EL0 CTR_EL0
#define SCTLR_A64_DZE       BIT64 (14)              // Enable EL0 DC ZVA
#define SCTLR_A32_V         BIT64 (13)              // Vectors
#define SCTLR_A64_EnDB      BIT64 (13)              // Enable Pointer Authentication
#define SCTLR_ALL_I         BIT64 (12)              // Enable I-Cache
#define SCTLR_A32_11        BIT64 (11)
#define SCTLR_A64_EOS       BIT64 (11)              // Exception Exit is Context-Synchronizing
#define SCTLR_ALL_EnRCTX    BIT64 (10)              // Enable EL0 RCTX instructions
#define SCTLR_A32_9         BIT64  (9)
#define SCTLR_A64_UMA       BIT64  (9)              // User Mask Access
#define SCTLR_ALL_SED       BIT64  (8)              // Disable EL0 SETEND (AArch32)
#define SCTLR_ALL_ITD       BIT64  (7)              // Disable EL0 IT instructions (AArch32)
#define SCTLR_A32_6         BIT64  (6)
#define SCTLR_A64_nAA       BIT64  (6)              // Enable Non-Aligned Access
#define SCTLR_ALL_CP15BEN   BIT64  (5)              // Enable EL0 DMB, DSB, ISB (AArch32)
#define SCTLR_A32_LSMAOE    BIT64  (4)              // Load/Store Multiple Atomicity/Ordering
#define SCTLR_A64_SA0       BIT64  (4)              // Enable Stack Alignment Check EL0
#define SCTLR_A32_nTLSMD    BIT64  (3)              // Don't trap Load/Store Multiple
#define SCTLR_A64_SA        BIT64  (3)              // Enable Stack Alignment Check EL1/2
#define SCTLR_ALL_C         BIT64  (2)              // Enable D-Cache
#define SCTLR_ALL_A         BIT64  (1)              // Enable Alignment Check
#define SCTLR_ALL_M         BIT64  (0)              // Enable MMU Stage 1
#define SCTLR_A32_RES0      (SCTLR_A32_63_32 | SCTLR_A32_27_26 | SCTLR_A32_24 | SCTLR_A32_21 | SCTLR_ALL_17 | SCTLR_A32_15_14 | SCTLR_A32_9 | SCTLR_A32_6)
#define SCTLR_A64_RES0      (SCTLR_A64_34 | SCTLR_ALL_17)
#define SCTLR_A32_RES1      (SCTLR_A32_22 | SCTLR_A32_11)
#define SCTLR_A64_RES1      (SFX_ULL(0))

#define TCR_A32_63_51       BIT64_RANGE (63, 51)
#define TCR_A64_63_60       BIT64_RANGE (63, 60)
#define TCR_A64_DS          BIT64 (59)
#define TCR_A64_TCMA1       BIT64 (58)
#define TCR_A64_TCMA0       BIT64 (57)
#define TCR_A64_E0PD1       BIT64 (56)              // TTBR1 Fault Control Address Translation
#define TCR_A64_E0PD0       BIT64 (55)              // TTBR0 Fault Control Address Translation
#define TCR_A64_NFD1        BIT64 (54)              // TTBR1 Non-Fault Translation Table Walk Disable
#define TCR_A64_NFD0        BIT64 (53)              // TTBR0 Non-Fault Translation Table Walk Disable
#define TCR_A64_TBID1       BIT64 (52)              // TTBR1 Top Byte Data Access Only Matching
#define TCR_A64_TBID0       BIT64 (51)              // TTBR0 Top Byte Data Access Only Matching
#define TCR_ALL_HWU162      BIT64 (50)              // TTBR1 Hardware Use Bit62
#define TCR_ALL_HWU161      BIT64 (49)              // TTBR1 Hardware Use Bit61
#define TCR_ALL_HWU160      BIT64 (48)              // TTBR1 Hardware Use Bit60
#define TCR_ALL_HWU159      BIT64 (47)              // TTBR1 Hardware Use Bit59
#define TCR_ALL_HWU062      BIT64 (46)              // TTBR0 Hardware Use Bit62
#define TCR_ALL_HWU061      BIT64 (45)              // TTBR0 Hardware Use Bit61
#define TCR_ALL_HWU060      BIT64 (44)              // TTBR0 Hardware Use Bit60
#define TCR_ALL_HWU059      BIT64 (43)              // TTBR0 Hardware Use Bit59
#define TCR_ALL_HPD1        BIT64 (42)              // TTBR1 Hierarchical Permission Disable
#define TCR_ALL_HPD0        BIT64 (41)              // TTBR0 Hierarchical Permission Disable
#define TCR_A32_40_32       BIT64_RANGE (40, 32)
#define TCR_A64_HD          BIT64 (40)              // Hardware Management Dirty Bit
#define TCR_A64_HA          BIT64 (39)              // Hardware Management Access Bit
#define TCR_A64_TBI1        BIT64 (38)              // TTBR1 Top Byte Ignored
#define TCR_A64_TBI0        BIT64 (37)              // TTBR0 Top Byte Ignored
#define TCR_A64_AS          BIT64 (36)              // ASID Size
#define TCR_A64_35          BIT64 (35)
#define TCR_A64_IPS         VAL64_SHIFT (7, 32)     // Intermediate Physical Address Size
#define TCR_A32_EAE         BIT64 (31)              // Extended Address Enable
#define TCR_A32_IMPL        BIT64 (30)              // Implementation Defined
#define TCR_A64_TG1_64K     VAL64_SHIFT (3, 30)     // TTBR1 Granule 64K
#define TCR_A64_TG1_4K      VAL64_SHIFT (2, 30)     // TTBR1 Granule 4K
#define TCR_A64_TG1_16K     VAL64_SHIFT (1, 30)     // TTBR1 Granule 16K
#define TCR_ALL_SH1_INNER   VAL64_SHIFT (3, 28)     // TTBR1 Inner Shareable
#define TCR_ALL_SH1_OUTER   VAL64_SHIFT (2, 28)     // TTBR1 Outer Shareable
#define TCR_ALL_SH1_NONE    VAL64_SHIFT (0, 28)     // TTBR1 Non-Shareable
#define TCR_ALL_ORGN1_WB_R  VAL64_SHIFT (3, 26)     // TTBR1 Normal Memory, Outer WB, Read-Allocate
#define TCR_ALL_ORGN1_WT_R  VAL64_SHIFT (2, 26)     // TTBR1 Normal Memory, Outer WT, Read-Allocate
#define TCR_ALL_ORGN1_WB_RW VAL64_SHIFT (1, 26)     // TTBR1 Normal Memory, Outer WB, Read-Allocate, Write-Allocate
#define TCR_ALL_ORGN1_NC    VAL64_SHIFT (0, 26)     // TTBR1 Normal Memory, Outer NC
#define TCR_ALL_IRGN1_WB_R  VAL64_SHIFT (3, 24)     // TTBR1 Normal Memory, Inner WB, Read-Allocate
#define TCR_ALL_IRGN1_WT_R  VAL64_SHIFT (2, 24)     // TTBR1 Normal Memory, Inner WT, Read-Allocate
#define TCR_ALL_IRGN1_WB_RW VAL64_SHIFT (1, 24)     // TTBR1 Normal Memory, Inner WB, Read-Allocate, Write-Allocate
#define TCR_ALL_IRGN1_NC    VAL64_SHIFT (0, 24)     // TTBR1 Normal Memory, Inner NC
#define TCR_ALL_EPD1        BIT64 (23)              // TTBR1 Disable Translations
#define TCR_ALL_A1          BIT64 (22)              // ASID Selector
#define TCR_A32_21_19       BIT64_RANGE (21, 19)
#define TCR_A64_T1SZ(X)     ((64 - (X)) << 16)
#define TCR_A32_15_14       BIT64_RANGE (15, 14)
#define TCR_A64_TG0_16K     VAL64_SHIFT (2, 14)     // TTBR0 Granule 16K
#define TCR_A64_TG0_64K     VAL64_SHIFT (1, 14)     // TTBR0 Granule 64K
#define TCR_A64_TG0_4K      VAL64_SHIFT (0, 14)     // TTBR0 Granule 4K
#define TCR_ALL_SH0_INNER   VAL64_SHIFT (3, 12)     // TTBR0 Inner Shareable
#define TCR_ALL_SH0_OUTER   VAL64_SHIFT (2, 12)     // TTBR0 Outer Shareable
#define TCR_ALL_SH0_NONE    VAL64_SHIFT (0, 12)     // TTBR0 Non-Shareable
#define TCR_ALL_ORGN0_WB_R  VAL64_SHIFT (3, 10)     // TTBR0 Normal Memory, Outer WB, Read-Allocate
#define TCR_ALL_ORGN0_WT_R  VAL64_SHIFT (2, 10)     // TTBR0 Normal Memory, Outer WT, Read-Allocate
#define TCR_ALL_ORGN0_WB_RW VAL64_SHIFT (1, 10)     // TTBR0 Normal Memory, Outer WB, Read-Allocate, Write-Allocate
#define TCR_ALL_ORGN0_NC    VAL64_SHIFT (0, 10)     // TTBR0 Normal Memory, Outer NC
#define TCR_ALL_IRGN0_WB_R  VAL64_SHIFT (3,  8)     // TTBR0 Normal Memory, Inner WB, Read-Allocate
#define TCR_ALL_IRGN0_WT_R  VAL64_SHIFT (2,  8)     // TTBR0 Normal Memory, Inner WT, Read-Allocate
#define TCR_ALL_IRGN0_WB_RW VAL64_SHIFT (1,  8)     // TTBR0 Normal Memory, Inner WB, Read-Allocate, Write-Allocate
#define TCR_ALL_IRGN0_NC    VAL64_SHIFT (0,  8)     // TTBR0 Normal Memory, Inner NC
#define TCR_ALL_EPD0        BIT64  (7)              // TTBR0 Disable Translations
#define TCR_A32_T2E         BIT64  (6)              // TTBCR2 Enable
#define TCR_A64_6           BIT64  (6)
#define TCR_A32_PD1         BIT64  (5)
#define TCR_A32_PD0         BIT64  (4)
#define TCR_A32_3           BIT64  (3)
#define TCR_A32_N           BIT64_RANGE (2, 0)
#define TCR_A64_T0SZ(X)     ((64 - (X)) << 0)
#define TCR_A32_RES0        (TCR_A32_63_51 | TCR_A32_40_32 | TCR_A32_21_19 | TCR_A32_15_14 | TCR_A32_3)
#define TCR_A64_RES0        (TCR_A64_63_60 | TCR_A64_35 | TCR_A64_6)

#define VTCR_63_34          BIT64_RANGE (63, 34)
#define VTCR_SL2            BIT64 (33)              // Stage 2 Starting Level
#define VTCR_DS             BIT64 (32)              // Stage 2 Descriptor Size
#define VTCR_31             BIT64 (31)
#define VTCR_NSA            BIT64 (30)              // Non-Secure Stage 2 Output Address Space
#define VTCR_NSW            BIT64 (29)              // Non-Secure Stage 2 Translation Table Space
#define VTCR_HWU62          BIT64 (28)              // Hardware Use
#define VTCR_HWU61          BIT64 (27)              // Hardware Use
#define VTCR_HWU60          BIT64 (26)              // Hardware Use
#define VTCR_HWU59          BIT64 (25)              // Hardware Use
#define VTCR_24_23          BIT64_RANGE (24, 23)
#define VTCR_HD             BIT64 (22)              // Hardware Update D-Flag
#define VTCR_HA             BIT64 (21)              // Hardware Update A-Flag
#define VTCR_20             BIT64 (20)
#define VTCR_VS             BIT64 (19)              // VMID Size
#define VTCR_SL0_L3         VAL64_SHIFT (3, 6)
#define VTCR_SL0_L0         VAL64_SHIFT (2, 6)
#define VTCR_SL0_L1         VAL64_SHIFT (1, 6)
#define VTCR_SL0_L2         VAL64_SHIFT (0, 6)
#define VTCR_T0SZ(X)        ((64 - (X)) << 0)
#define VTCR_RES0           (VTCR_63_34 | VTCR_24_23 | VTCR_20)
#define VTCR_RES1           (VTCR_31)

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

#define MDCR_63_37          BIT64_RANGE (63, 37)
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
#define MDCR_RES0           (MDCR_63_37 | MDCR_35_30 | MDCR_22_20 | MDCR_18 | MDCR_16_15)
#define MDCR_RES1           (SFX_ULL(0))

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
#define HCR_ID              BIT64 (33)              // Stage 2 Inst Cache Disable
#define HCR_CD              BIT64 (32)              // Stage 2 Data Cache Disable
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
#define HCR_VM              BIT64  (0)              // Enable MMU Stage 2

#define HCRX_63_5           BIT64_RANGE (63, 12)
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
#define HCRX_RES0           (HCRX_63_12)

#define SCR_63              BIT64 (63)
#define SCR_NSE             BIT64 (62)              // Security State of EL2
#define SCR_61_49           BIT64_RANGE (61, 49)
#define SCR_GPF             BIT64 (48)              // Trap Granule Protection Faults
#define SCR_47_42           BIT64_RANGE (47, 42)
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
#define SCR_RES0            (SCR_61_49 | SCR_47_42 | SCR_24_22 | SCR_6)
#define SCR_RES1            (SCR_5_4)
