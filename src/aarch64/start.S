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

#define PTE_ATTR_P      BIT  (0)    // Present
#define PTE_ATTR_nL     BIT  (1)    // Not Large
#define PTE_ATTR_CA(X)  ((X) << 2)  // Cacheability
#define PTE_ATTR_SH(X)  ((X) << 8)  // Shareability
#define PTE_ATTR_A      BIT (10)    // Accessed
#define PTE_ATTR_nG     BIT (11)    // Not Global

.globl                  __init_bsp

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

/*
 * Initialization Code
 */
.section .init

__init_bsp:
                        // Determine image boundaries
                        adrp    x22, NOVA_HPAS
                        adrp    x23, NOVA_HPAE
                        mov     x24, LINK_ADDR
                        sub     x24, x24, x22

                        // Refuse invalid load address
                        and     x2, x22, #OFFS_MASK (1)
                        cbnz    x2, .

                        // Clean image to PoC
                        mov     x0, x22
                        sub     x1, x23, x22
                        DCACHE  cvac

                        msr     daifset, #0xf
                        msr     spsel,   #0x1

                        // Enable I$, D$, Disable MMU
                        mrs     x0, sctlr_el2
                        orr     x0, x0, #SCTLR_I
                        orr     x0, x0, #SCTLR_C
                        bic     x0, x0, #SCTLR_M
                        msr     sctlr_el2, x0
                        isb

                        // Zero BSS
                        adr     x0, ZERO_HPAS
.Lbss_loop:             str     xzr, [x0], #__SIZEOF_POINTER__
                        cmp     x0, x23
                        blo     .Lbss_loop

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

                        // Switch to boot stack
                        adr     x0, STACK
                        mov     sp, x0

                        // Jump high
                        ldr     x0, =.Lhigh
                        br      x0

.text

.Lhigh:
                        // Initialize kmem offset
                        adr     x0, __kmem_offset
                        str     x24, [x0]

                        // Early initialization
                        bl      preinit

                        // General initialization
                        bl      init

                        b       .
