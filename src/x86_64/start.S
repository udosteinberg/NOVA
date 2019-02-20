/*
 * Startup Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "macros.hpp"
#include "memattr.hpp"
#include "memory.hpp"
#include "paging.hpp"
#include "selectors.hpp"

.globl                  __init_bsp, __init_aps, __desc_gdt__

/*
 * Macros
 */
.macro                  PTE_FIX L, PTAB, VIRT, PHYS, ATTR
                        lea     (\PHYS) + (\ATTR)(%ebp), %ecx
                        mov     %ecx, (\PTAB) + SIZE * ((\VIRT) >> ((\L) * PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))(%ebp)
.endm

.macro                  PTE_REL L, PTAB, VIRT, PHYS, ATTR
                        lea     (\PHYS) + (\ATTR)(%ebp), %ecx
                        lea     (\VIRT)(%ebp), %edx
                        shr     $((\L) * PTE_BPL + PAGE_BITS), %edx
                        and     $(BIT (PTE_BPL) - 1), %edx
                        mov     %ecx, (\PTAB)(%ebp, %edx, SIZE)
.endm

.macro                  INIT_PAGING
                        mov     $0x277, %ecx
                        mov     $(PAT_VAL >> 32), %edx
                        mov     $(PAT_VAL & BIT_RANGE (31, 0)), %eax
                        wrmsr
                        mov     $0xc0000080, %ecx
                        xor     %edx, %edx
                        mov     $(EFER_NXE | EFER_LME | EFER_SCE), %eax
                        wrmsr
                        lea     STACK(%ebp), %esp
                        lea     PT3S_HPAS(%ebp), %ecx
                        mov     $(CR4_OSXMMEXCPT | CR4_OSFXSR | CR4_PGE | CR4_PAE | CR4_PSE | CR4_DE), %edx
                        mov     $(CR0_PG | CR0_WP | CR0_NE | CR0_MP | CR0_PE), %eax
                        mov     %edx, %cr4
                        mov     %ecx, %cr3
                        mov     %eax, %cr0
.endm

/*
 * Initialization Code
 */
.section .init

/*
 * 32-bit Startup Code: Bootstrap Processor (BSP)
 */
.code32

__init_bsp:             cli
                        cld

                        // Determine relocation offset
                        lea     16(%ebx), %esp
                        call    1f
1:                      pop     %ebp
                        sub     $1b, %ebp

                        // Apply relocations
                        add     %ebp, 0 + __addr_gdt(%ebp)
                        add     %ebp, 2 + __jump_aps(%ebp)

                        // Configure Page Tables: Load Section (2M page)
                        PTE_REL 3, PT3S_HPAS, NOVA_HPAS, PT2L_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P
                        PTE_REL 2, PT2L_HPAS, NOVA_HPAS, PT1L_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P
                        PTE_REL 1, PT1L_HPAS, NOVA_HPAS, NOVA_HPAS, ATTR_S | ATTR_D | ATTR_A | ATTR_W | ATTR_P

                        // FIXME: AP Startup Code mapping
                        PTE_REL 1, PT1L_HPAS, 0, 0, ATTR_S | ATTR_D | ATTR_A | ATTR_W | ATTR_P

                        // Configure Page Tables: Link Section (2M pages)
                        PTE_FIX 3, PT3S_HPAS, LINK_ADDR, PT2H_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P
                        PTE_FIX 2, PT2H_HPAS, LINK_ADDR, PT1H_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P

                        lea     PT1H_HPAS + SIZE * (LINK_ADDR >> (PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))(%ebp), %edx
                        lea     NOVA_HPAS + (ATTR_G | ATTR_S | ATTR_D | ATTR_A | ATTR_W | ATTR_P)(%ebp), %ecx
                        lea     NOVA_HPAE(%ebp), %eax
1:                      mov     %ecx, (%edx)
                        add     $SIZE, %edx
                        add     $(BIT (PTE_BPL + PAGE_BITS)), %ecx
                        cmp     %eax, %ecx
                        jb      1b

                        INIT_PAGING
.code64
                        lea     __init_all(%rbp), %eax
                        lgdt    __desc_gdt(%rbp)
                        pushq   $SEL_KERN_CODE
                        pushq   %rax
                        lretq

/*
 * 16-bit Startup Code: Application Processors (APs)
 */
.code16

__init_aps:             mov     %cs:__addr_gdt - __init_aps, %ebp
                        sub     $__boot_gdt, %ebp
                        INIT_PAGING
                        lgdtl   %cs:__desc_gdt - __init_aps
__jump_aps:             ljmpl   $SEL_KERN_CODE, $__init_all

/*
 * Global Descriptor Table (GDT)
 */
.balign                 0x8, 0x90

__boot_gdt:             .word   0
__desc_gdt:             .word   __boot_gdt__ - __boot_gdt - 1
__addr_gdt:             .long   __boot_gdt
__desc_gdt__:           .quad   0x00a09b0000000000
                        .quad   0x00a0930000000000
                        .quad   0x00a0fb0000000000
                        .quad   0x00a0f30000000000
__boot_gdt__:

/*
 * 64-bit Startup Code: All Processors
 */
.code64

__init_all:             lea     high, %rax
                        jmp     *%rax

.text

high:                   mov     $SEL_KERN_DATA, %ecx
                        mov     %cx, %ss
                        mov     $SEL_USER_DATA, %ecx
                        mov     %cx, %ds
                        mov     %cx, %es
                        mov     %cx, %fs
                        mov     %cx, %gs

                        test    %rbx, %rbx
                        je      2f

                        // Note: %rbp has upper bits 0, not sign-extended
                        lea     NOVA_HPAS(%ebp), %rax
                        lea     LINK_ADDR, %rdi
                        sub     %rax, %rdi
                        mov     %rbx, %rsi
                        call    init
                        jmp     3f

1:                      pause
2:                      xchg    %rbx, boot_lock
                        test    %rbx, %rbx
                        je      1b

3:                      call    kern_ptab_setup
                        mov     %rax, %cr3
                        lea     STCK_TOP, %rsp
                        jmp     bootstrap

.section .init

/*
 * Multiboot Header
 */
.balign                 0x8

                        .long   0x1badb002
                        .long   BIT (0)
                        .long  -0x1badb002 - BIT (0)
