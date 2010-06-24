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

/*
 * AP/RLP Startup Code
 */
.code16

__start_ap:             lgdtl   %cs:__gdt_desc - __start_ap
                        mov     $1, %dx
                        lmsw    %dx
                        ljmpl   $SEL_KERN_CODE, $__start_rlp

__gdt_desc:             .word   __boot_gdt__ - __boot_gdt - 1
                        .long   __boot_gdt

.code32

__start_rlp:            mov     $SEL_KERN_DATA, %ecx
                        mov     %ecx, %ss
                        mov     $SEL_USER_DATA, %ecx
                        mov     %ecx, %ds
                        mov     %ecx, %es
                        mov     %ecx, %fs
                        mov     %ecx, %gs
                        mov     $STACK, %esp

                        xor     %eax, %eax
1:                      pause
                        xchg    %eax, boot_lock_p
                        test    %eax, %eax
                        jz      1b

                        call    init_rlp

                        jmp     __start_all

/*
 * BSP/ILP Startup Code
 */
__start_bsp:            xor     %ebp, %ebp
                        cmp     $0x2badb002, %eax
                        cmovne  %ebp, %ebx

1:                      lgdt    __boot_gdt + 2
                        ljmp    $SEL_KERN_CODE, $2f

2:                      mov     $SEL_KERN_DATA, %ecx
                        mov     %ecx, %ss
                        mov     $SEL_USER_DATA, %ecx
                        mov     %ecx, %ds
                        mov     %ecx, %es
                        mov     %ecx, %fs
                        mov     %ecx, %gs
                        mov     $STACK, %esp

                        mov     %ebx, %eax
                        call    init_ilp

/*
 * Common Startup Code
 */
__start_all:            call    kern_ptab_setup
                        jmp     1f

.text

1:                      mov     %eax, %cr3
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %esp
                        jmp     bootstrap
