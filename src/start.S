/*
 * Startup Code
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "memory.h"
#include "selectors.h"

/*
 * Initialization Code
 */
.section .init

.globl                  __start_ap
.globl                  __start_rlp
.globl                  __start_bsp

/*
 * Multiboot Header
 */
__boot_header:          .long   0x1badb002
                        .long   0x0
                        .long  -0x1badb002

/*
 * Global Descriptor Table (GDT)
 */
.align                  8, 0x90
__boot_gdt:             .word   0
                        .word   __boot_gdt__ - __boot_gdt - 1
                        .long   __boot_gdt
                        .quad   0x00cf9b000000ffff      /* Kernel Code */
                        .quad   0x00cf93000000ffff      /* Kernel Data */
                        .quad   0x00cffb000000ffff      /* User Code */
                        .quad   0x00cff3000000ffff      /* User Data */
__boot_gdt__:

.macro                  INIT_STATE
                        mov     $SEL_KERN_DATA, %ecx
                        mov     %ecx, %ss
                        mov     $SEL_USER_DATA, %ecx
                        mov     %ecx, %ds
                        mov     %ecx, %es
                        mov     %ecx, %fs
                        mov     %ecx, %gs
                        mov     $STACK, %esp
.endm

/*
 * AP Startup Code
 */
.code16

__start_ap:             lgdtl   %cs:__gdt_desc - __start_ap
                        mov     $1, %dx
                        lmsw    %dx
                        ljmpl   $SEL_KERN_CODE, $__start_rlp

__gdt_desc:             .word   __boot_gdt__ - __boot_gdt - 1
                        .long   __boot_gdt

.code32

__start_rlp:            INIT_STATE

                        xor     %edx, %edx
1:                      pause
                        xchg    %edx, boot_lock_p
                        test    %edx, %edx
                        jz      1b

                        jmp     __start_all

/*
 * BSP Startup Code
 */
__start_bsp:            lgdt    __boot_gdt + 2
                        ljmp    $SEL_KERN_CODE, $1f

1:                      INIT_STATE

                        cmp     $0x2badb002, %eax
                        jne     1f
                        testb   $0x4, (%ebx)
                        je      1f
                        mov     0x10(%ebx), %eax
                        call    cmdline

1:                      movl    $0xe3, LVL2
                        mov     $(LINK_P + 0x1e3), %eax
                        mov     $(LVL2 + LINK_ADDR >> 20), %edi
1:                      stosl
                        add     $0x400000, %eax
                        cmp     $LINK_E, %eax
                        jb      1b

/*
 * Common Startup Code
 */
__start_all:            mov     $LVL2, %edx
                        mov     %edx, %cr3
                        mov     $0x698, %edx
                        mov     %edx, %cr4
                        mov     $0x8001002b, %edx
                        mov     %edx, %cr0

                        test    %eax, %eax
                        je      1f
                        mov     %ebx, %eax
                        call    init

1:                      call    kern_ptab_setup
                        jmp     1f

.text

1:                      mov     %eax, %cr3
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %esp
                        jmp     bootstrap
