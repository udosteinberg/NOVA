/*
 * Startup Code
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

#include "arch.hpp"
#include "memattr.hpp"
#include "memory.hpp"
#include "multiboot.hpp"

#define PTE_ATTR_P      BIT  (0)    // Present
#define PTE_ATTR_nL     BIT  (1)    // Not Large
#define PTE_ATTR_CA(X)  ((X) << 2)  // Cacheability
#define PTE_ATTR_SH(X)  ((X) << 8)  // Shareability
#define PTE_ATTR_A      BIT (10)    // Accessed
#define PTE_ATTR_nG     BIT (11)    // Not Global

.globl                  __init_bsp, __init_psci, __init_spin

/*
 * Macros
 */
.macro                  DCACHE  OP
                        mrs     x3, ctr_el0
                        ubfm    x3, x3, #16, #19
                        mov     x2, #4
                        lsl     x2, x2, x3
                        add     x1, x0, x1
                        sub     x3, x2, #1
                        bic     x0, x0, x3
1:                      dc     \OP, x0
                        add     x0, x0, x2
                        cmp     x0, x1
                        blo     1b
                        dsb     sy
.endm

.macro                  PTE_FIX L, PTAB, VIRT, PHYS, ATTR
                        adr     x0, (\PTAB) + __SIZEOF_POINTER__ * ((\VIRT) >> ((\L) * PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))
                        adr     x1, (\PHYS) + (\ATTR)
                        str     x1, [x0]
.endm

.macro                  PTE_REL L, PTAB, VIRT, PHYS, ATTR
                        adr     x0, (\PTAB)
                        adr     x1, (\PHYS) + (\ATTR)
                        adr     x2, (\VIRT)
                        lsr     x2, x2, (\L) * PTE_BPL + PAGE_BITS
                        and     x2, x2, BIT (PTE_BPL) - 1
                        add     x0, x0, x2, lsl #3
                        str     x1, [x0]
.endm

.macro                  RECTIME SYMBOL
                        mrs     x14, cntpct_el0
                        adr     x15, \SYMBOL - OFFSET
                        str     x14, [x15]
.endm

/*
 * Initialization Code
 */
.section .init

__init_bsp:             RECTIME multiboot_t0

                        // Store boot parameters
                        adr     x3, multiboot_p0 - OFFSET
                        str     x0, [x3]
                        adr     x3, multiboot_p1 - OFFSET
                        str     x1, [x3]
                        adr     x3, multiboot_p2 - OFFSET
                        str     x2, [x3]

                        // Determine image boundaries
                        adrp    x22, NOVA_HPAS
                        adrp    x23, NOVA_HPAE
                        mov     x24, LINK_ADDR
                        sub     x24, x24, x22

                        // Refuse invalid load address
                        and     x2, x22, #OFFS_MASK (1)
                        cbnz    x2, .
.Lmb2_launch:
                        // Multiboot v2 Launch
                        ldr     x2, =MULTIBOOT_V2_LOADER
                        cmp     x2, x0
                        bne     .Lmb1_launch

                        // Prepare tag loop
                        ldr     w4, [x1]            // Total Size
                        add     x3, x1, #8          // Tag Ptr
                        add     x2, x1, x4          // Tag End
                        mov     x1, xzr             // EFI Sys
                        mov     x0, xzr             // EFI Img
.Lmb2_tag_loop:
                        ldr     w4, [x3]

                        cmp     x4, #MULTIBOOT_V2_INFO_END
                        beq     .Lmb2_end

                        cmp     x4, #MULTIBOOT_V2_INFO_CMD
                        bne     .Lmb2_not_cmd
                        adr     x5, multiboot_cl - OFFSET
                        add     x4, x3, #8
                        str     x4, [x5]
                        b       .Lmb2_nxt_tag
.Lmb2_not_cmd:
                        cmp     x4, #MULTIBOOT_V2_INFO_MOD
                        bne     .Lmb2_not_mod
                        adr     x5, multiboot_ra - OFFSET
                        ldr     x4, [x5]
                        cbnz    x4, .Lmb2_nxt_tag
                        ldr     w4, [x3, #8]
                        str     x4, [x5]
                        b       .Lmb2_nxt_tag
.Lmb2_not_mod:
                        cmp     x4, #MULTIBOOT_V2_INFO_SYS
                        bne     .Lmb2_not_sys
                        ldr     x1, [x3, #8]
                        b       .Lmb2_nxt_tag
.Lmb2_not_sys:
                        cmp     x4, #MULTIBOOT_V2_INFO_IMG
                        bne     .Lmb2_not_img
                        ldr     x0, [x3, #8]
                        b       .Lmb2_nxt_tag
.Lmb2_not_img:
                        ldr     x5, =MULTIBOOT_V2_INFO_KMEM
                        cmp     x4, x5
                        bne     .Lmb2_nxt_tag
                        mov     x5, #0x20000000
                        sub     x4, x23, x22
                        sub     x5, x5, x4
                        ldr     x4, [x3, #8]
                        cmp     x4, x5
                        csel    x4, x4, x5, lo
                        bic     x4, x4, #OFFS_MASK (1)
                        add     x23, x23, x4
.Lmb2_nxt_tag:
                        ldr     w4, [x3, #4]
                        add     x3, x3, x4
                        add     x3, x3, #7
                        bic     x3, x3, #7
                        cmp     x3, x2
                        blo     .Lmb2_tag_loop
.Lmb2_end:
                        // Initialize UEFI
                        cbz     x0, .Lmb_end
                        cbz     x1, .Lmb_end
                        adr     x2, uefi_info - OFFSET
                        adr     lr, .Lmb_end
                        b       uefi_init
.Lmb1_launch:
                        // Multiboot v1 Launch
                        ldr     x2, =MULTIBOOT_V1_LOADER
                        cmp     x2, x0
                        bne     .Llgc_launch
                        ldr     w3, [x1]

                        tbz     x3, #MULTIBOOT_V1_INFO_CMD, .Lmb1_not_cmd
                        adr     x5, multiboot_cl - OFFSET
                        ldr     w4, [x1, #16]
                        str     x4, [x5]
.Lmb1_not_cmd:
                        tbz     x3, #MULTIBOOT_V1_INFO_MOD, .Lmb_end
                        adr     x5, multiboot_ra - OFFSET
                        ldr     w4, [x1, #20]
                        cbz     x4, .Lmb_end
                        ldr     w4, [x1, #24]
                        ldr     w4, [x4]
                        str     x4, [x5]
.Llgc_launch:
                        // Legacy Launch
                        adr     x5, multiboot_ra - OFFSET
                        str     x1, [x5]
.Lmb_end:
                        RECTIME multiboot_t1

                        // Store image end address
                        adr     x5, multiboot_ea - OFFSET
                        str     x23, [x5]

                        // Clean image to PoC
                        mov     x0, x22
                        sub     x1, x23, x22
                        DCACHE  cvac

                        mov     x19, #-1
                        b       .Linit_all

__init_psci:            mov     x19, x0
                        b       .Linit_all

__init_spin:            mrs     x0, mpidr_el1
                        and     x19, x0, #0xff

.Linit_all:             msr     daifset, #0xf
                        msr     spsel,   #0x1

                        // Enable I$, D$, Disable MMU
                        mrs     x0, sctlr_el2
                        orr     x0, x0, #SCTLR_I
                        orr     x0, x0, #SCTLR_C
                        bic     x0, x0, #SCTLR_M
                        msr     sctlr_el2, x0
                        isb

                        // Skip ahead if not BSP init
                        tbz     x19, #62, .Lbss_skip

                        // Zero BSS
                        adr     x0, ZERO_HPAS
.Lbss_loop:             str     xzr, [x0], #__SIZEOF_POINTER__
                        cmp     x0, x23
                        blo     .Lbss_loop
.Lbss_skip:
                        // Determine PA range (0-5)
                        mov     x20, #5
                        mrs     x21, id_aa64mmfr0_el1
                        and     x21, x21, #7
                        cmp     x21, x20
                        csel    x21, x20, x21, hi

                        // Determine current execution level
                        mrs     x20, currentel

                        // Check if we launched in EL3
                        cmp     x20, #SPSR_EL3
                        beq     .Lel3

                        // Check if we launched in EL2
                        cmp     x20, #SPSR_EL2
                        beq     .Lel2

                        // We launched in EL1 or EL0
                        ret
.Lel3:
                        // Configure EL3 TCR: 4K pages, 48bit VA, HW-supported PA
                        mov     x0, #(TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | TCR_T0SZ (48))
                        orr     x2, x0, x21, lsl #16
                        msr     tcr_el3, x2

                        // Configure EL3 MAIR
                        ldr     x0, =MAIR_VAL
                        msr     mair_el3, x0

                        // Invalidate EL3 TLB
                        tlbi    alle3
                        dsb     nsh
                        isb

                        // Configure EL3 execution controls via SCTLR
                        mrs     x0, sctlr_el3
                        orr     x0, x0, #(SCTLR_SA | SCTLR_C | SCTLR_A)
                        orr     x0, x0, #(SCTLR_I)
                        msr     sctlr_el3, x0
                        isb

                        // Configure EL2 execution controls via SCR
                        mrs     x0, scr_el3
                        orr     x0, x0, #SCR_RW
                        orr     x0, x0, #SCR_RES1
                        orr     x0, x0, #SCR_NS
                        msr     scr_el3, x0

                        // Configure EL2 initial SCTLR
                        msr     sctlr_el2, xzr

                        // Return to EL2
                        mov     x0, #(SPSR_D | SPSR_A | SPSR_I | SPSR_F | SPSR_EL2 | SPSR_SP)
                        adr     x1, .Lel2
                        msr     spsr_el3, x0
                        msr     elr_el3, x1
                        eret
.Lel2:
                        // Skip ahead if not BSP init
                        tbz     x19, #62, .Lpte_skip

                        // Configure Page Tables: Load Section (2M page)
                        PTE_REL  3, PT3S_HPAS, NOVA_HPAS, PT2L_HPAS, PTE_ATTR_nL | PTE_ATTR_P
                        PTE_REL  2, PT2L_HPAS, NOVA_HPAS, PT1L_HPAS, PTE_ATTR_nL | PTE_ATTR_P
                        PTE_REL  1, PT1L_HPAS, NOVA_HPAS, NOVA_HPAS, PTE_ATTR_nG | PTE_ATTR_A | PTE_ATTR_SH (3) | PTE_ATTR_CA (CA_MEM_WB) | PTE_ATTR_P

                        // Configure Page Tables: Link Section (2M pages)
                        PTE_FIX  3, PT3S_HPAS, LINK_ADDR, PT2H_HPAS, PTE_ATTR_nL | PTE_ATTR_P
                        PTE_FIX  2, PT2H_HPAS, LINK_ADDR, PT1H_HPAS, PTE_ATTR_nL | PTE_ATTR_P

                        adr     x1, PT1H_HPAS + __SIZEOF_POINTER__ * (LINK_ADDR >> (PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))
                        adr     x0, NOVA_HPAS + (PTE_ATTR_nG | PTE_ATTR_A | PTE_ATTR_SH (3) | PTE_ATTR_CA (CA_MEM_WB) | PTE_ATTR_P)
.Lpte_loop:             str     x0, [x1], #__SIZEOF_POINTER__
                        add     x0, x0, #PAGE_SIZE (1)
                        cmp     x0, x23
                        blo     .Lpte_loop
                        dsb     ishst

                        // Invalidate stale cache lines
                        adr     x0, ZERO_HPAS
                        sub     x1, x23, x0
                        DCACHE  ivac
.Lpte_skip:
                        // Configure EL2 TCR: 4K pages, 48bit VA, HW-supported PA
                        mov     x0, #(TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | TCR_T0SZ (48))
                        orr     x0, x0, #BIT (31)
                        orr     x0, x0, #BIT (23)
                        orr     x2, x0, x21, lsl #16
                        msr     tcr_el2, x2

                        // Configure EL2 MAIR
                        ldr     x0, =MAIR_VAL
                        msr     mair_el2, x0

                        // Configure EL2 TTBR
                        adrp    x0, PT3S_HPAS
                        msr     ttbr0_el2, x0

                        // Invalidate EL2 TLB
                        tlbi    alle2
                        dsb     nsh
                        isb

                        // Configure EL2 execution controls via SCTLR
                        mrs     x0, sctlr_el2
                        orr     x0, x0, #(SCTLR_SA | SCTLR_C | SCTLR_A | SCTLR_M)
                        orr     x0, x0, #(SCTLR_I)
                        msr     sctlr_el2, x0
                        isb

                        RECTIME multiboot_t2

                        // Switch to boot stack
                        adr     x0, STACK
                        mov     sp, x0

                        // Jump high
                        ldr     x0, =.Lhigh
                        br      x0

.text

.Lhigh:
                        // Acquire boot lock
                        adr     x0, __boot_lock
.Llock_retry_tu:        ldaxr   w1, [x0]
                        add     w2, w1, #BIT (16)
                        stxr    w3, w2, [x0]
                        cbnz    w3, .Llock_retry_tu
                        eor     w3, w1, w1, ror #16
                        cbz     w3, .Llock_end
                        sevl
.Llock_retry_tw:        wfe
                        ldaxrh  w2, [x0]
                        eor     w3, w2, w1, lsr #16
                        cbnz    w3, .Llock_retry_tw
.Llock_end:
                        // Skip ahead if not BSP
                        tbz     x19, #63, .Lskip_bsp

                        // Initialize kmem offset
                        adr     x0, __kmem_offset
                        str     x24, [x0]

                        // Early initialization
                        bl      preinit

                        // General initialization
                        bl      init
                        mov     x19, x0
.Lskip_bsp:
                        // Switch to CPU-local page table
                        mov     x0, x19
                        bl      kern_ptab_setup
                        msr     ttbr0_el2, x0
                        isb

                        // Switch to CPU-local stack
                        adrp    x0, DSTK_TOP
                        mov     sp, x0

                        // Invalidate TLB
                        tlbi    alle2
                        dsb     nsh
                        isb

                        mov     x0, x19
                        ubfx    x1, x20, #2, #2
                        b       bootstrap
