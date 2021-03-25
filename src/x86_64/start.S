/*
 * Startup Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "arch.hpp"
#include "memattr.hpp"
#include "memory.hpp"
#include "multiboot.hpp"
#include "paging.hpp"
#include "selectors.hpp"

.globl                  __boot_cl, __boot_ra, __boot_p0, __boot_p1, __boot_p2, __boot_ts, __init_bsp, __init_aps, __wake_vec, __desc_gdt__

/*
 * Macros
 */
.macro                  PTE_FIX L, PTAB, VIRT, PHYS, ATTR
                        lea     (\PHYS) + (\ATTR)(%ebp), %ecx
                        mov     %ecx, (\PTAB) + __SIZEOF_POINTER__ * ((\VIRT) >> ((\L) * PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))(%ebp)
.endm

.macro                  PTE_REL L, PTAB, VIRT, PHYS, ATTR
                        lea     (\PHYS) + (\ATTR)(%ebp), %ecx
                        lea     (\VIRT)(%ebp), %edx
                        shr     $((\L) * PTE_BPL + PAGE_BITS), %edx
                        and     $(BIT (PTE_BPL) - 1), %edx
                        mov     %ecx, (\PTAB)(%ebp, %edx, __SIZEOF_POINTER__)
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
 * 32/64-bit Startup Code: Bootstrap Processor (BSP)
 */
.code32

__init_bsp:             // Check if we launched via Multiboot v2
                        cmp     $MULTIBOOT_V2_LOADER, %eax
                        je      .Lvalid_launch

                        // Check if we launched via Multiboot v1
                        cmp     $MULTIBOOT_V1_LOADER, %eax
                        je      .Lvalid_launch

                        hlt
                        jmp     .
.Lvalid_launch:
                        // Determine relocation offset
                        xchg    %ebx, %esp
                        pop     %esi
                        call    .Lret_addr
.Lret_addr:             pop     %ebp
                        push    %esi
            dec %esi;   sub     $.Lret_addr, %ebp
                        xchg    %ebx, %esp

                        // Store boot parameters
                        mov     %eax, __boot_p0(%ebp)
                        mov     %ebx, __boot_p1(%ebp)

                        // Store boot time
                        rdtsc
                        mov     %eax, 0 + __boot_ts(%ebp)
                        mov     %edx, 4 + __boot_ts(%ebp)

                        // Apply relocations
                        add     %ebp, 0 + __addr_gdt(%ebp)
                        add     %ebp, 2 + __jump_aps(%ebp)
.Lmb2_launch:
                        // Multiboot v2 Launch
                        cmpl    $MULTIBOOT_V2_LOADER, __boot_p0(%ebp)
                        jne     .Lmb1_launch

                        // Prepare tag loop
                        lea     8(%ebx), %ecx       // Tag Ptr
                        add     0(%ebx), %ebx       // Tag End
                        xor     %esi, %esi          // EFI Sys
                        xor     %edi, %edi          // EFI Img
.Lmb2_tag_loop:
                        cmpb    $MULTIBOOT_V2_INFO_END, (%ecx)
                        je      .Lmb2_end

                        cmpb    $MULTIBOOT_V2_INFO_CMD, (%ecx)
                        jne     .Lmb2_not_cmd
                        lea     8(%ecx), %eax
                        mov     %eax, __boot_cl(%ebp)
                        jmp     .Lmb2_nxt_tag
.Lmb2_not_cmd:
                        cmpb    $MULTIBOOT_V2_INFO_MOD, (%ecx)
                        jne     .Lmb2_not_mod
                        cmpl    $0x0, __boot_ra(%ebp)
                        jne     .Lmb2_nxt_tag
                        mov     8(%ecx), %eax
                        mov     %eax, __boot_ra(%ebp)
                        jmp     .Lmb2_nxt_tag
.Lmb2_not_mod:
                        cmpb    $MULTIBOOT_V2_INFO_SYS, (%ecx)
                        jne     .Lmb2_not_sys
                        mov     8(%ecx), %esi
                        jmp     .Lmb2_nxt_tag
.Lmb2_not_sys:
                        cmpb    $MULTIBOOT_V2_INFO_IMG, (%ecx)
                        jne     .Lmb2_nxt_tag
                        mov     8(%ecx), %edi
.Lmb2_nxt_tag:
                        mov     4(%ecx), %eax
                        lea     7(%ecx,%eax), %ecx
                        and     $~7, %ecx
                        cmp     %ebx, %ecx
                        jb      .Lmb2_tag_loop
.Lmb2_end:
                        // Initialize UEFI
                        test    %edi, %edi
                        jz      .Lmb_end
                        test    %esi, %esi
                        jz      .Lmb_end
                        lea     __uefi_info(%ebp), %edx
                        call    __uefi_init
                        jmp     .Lmb_end
.Lmb1_launch:
                        // Multiboot v1 Launch
                        cmpl    $MULTIBOOT_V1_LOADER, __boot_p0(%ebp)
                        jne     .Lmb_end

                        testb   $BIT (MULTIBOOT_V1_INFO_CMD), (%ebx)
                        jz      .Lmb1_not_cmd
                        mov     16(%ebx), %eax
                        mov     %eax, __boot_cl(%ebp)
.Lmb1_not_cmd:
                        testb   $BIT (MULTIBOOT_V1_INFO_MOD), (%ebx)
                        jz      .Lmb_end
                        mov     20(%ebx), %eax
                        test    %eax, %eax
                        jz      .Lmb_end
                        mov     24(%ebx), %eax
                        mov     (%eax), %eax
                        mov     %eax, __boot_ra(%ebp)
.Lmb_end:
                        // Ensure flags are in a defined state (after UEFI init)
                        cli
                        cld

                        // Configure Page Tables: Load Section (2M page)
                        PTE_REL 3, PT3S_HPAS, NOVA_HPAS, PT2L_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P
                        PTE_REL 2, PT2L_HPAS, NOVA_HPAS, PT1L_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P
                        PTE_REL 1, PT1L_HPAS, NOVA_HPAS, NOVA_HPAS, ATTR_S | ATTR_D | ATTR_A | ATTR_W | ATTR_P

                        // FIXME: AP Startup Code mapping
                        PTE_REL 1, PT1L_HPAS, 0, 0, ATTR_S | ATTR_D | ATTR_A | ATTR_W | ATTR_P

                        // Configure Page Tables: Link Section (2M pages)
                        PTE_FIX 3, PT3S_HPAS, LINK_ADDR, PT2H_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P
                        PTE_FIX 2, PT2H_HPAS, LINK_ADDR, PT1H_HPAS, ATTR_A | ATTR_U | ATTR_W | ATTR_P

                        lea     PT1H_HPAS + __SIZEOF_POINTER__ * (LINK_ADDR >> (PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))(%ebp), %edx
                        lea     NOVA_HPAS + (ATTR_G | ATTR_S | ATTR_D | ATTR_A | ATTR_W | ATTR_P)(%ebp), %ecx
                        lea     NOVA_HPAE(%ebp), %eax
.Lpte_loop:             mov     %ecx, (%edx)
                        add     $__SIZEOF_POINTER__, %edx
                        add     $(BIT (PTE_BPL + PAGE_BITS)), %ecx
                        cmp     %eax, %ecx
                        jb      .Lpte_loop

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

__init_aps:             jmp     .Lskip_vec
__wake_vec:             orb     $1, %bl
.Lskip_vec:             mov     %cs:__addr_gdt - __init_aps, %ebp
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

__boot_cl:              .quad   0
__boot_ra:              .quad   0
__boot_p0:              .quad   0
__boot_p1:              .quad   0
__boot_p2:              .quad   0
__boot_ts:              .quad   0

/*
 * 64-bit Startup Code: All Processors
 */
.code64

__init_all:             lea     .Lhigh, %rax
                        jmp     *%rax

.text

.Lhigh:                 mov     $SEL_KERN_DATA, %ecx
                        mov     %cx, %ss
                        xor     %ecx, %ecx
                        mov     %cx, %ds
                        mov     %cx, %es
                        mov     %cx, %fs
                        mov     %cx, %gs

                        test    %rbx, %rbx
                        jz      .Linit_end

                        // Note: %rbp has upper bits 0, not sign-extended
                        lea     NOVA_HPAS(%ebp), %rax
                        lea     LINK_ADDR, %rdi
                        sub     %rax, %rdi
                        call    init
.Linit_end:
                        // Acquire boot lock
                        mov     $BIT (8), %ecx
                   lock xadd    %cx, __boot_lock
.Llock_retry:           cmp     %ch, %cl
                        jz      .Llock_end
                        pause
                        mov     __boot_lock, %cl
                        jmp     .Llock_retry
.Llock_end:
                        mov     $0x1, %eax
                        cpuid
                        shr     $0x18, %ebx
                        mov     %rbx, %rdi
                        call    kern_ptab_setup
                        mov     %rax, %cr3
                        lea     DSTK_TOP, %rsp
                        jmp     bootstrap
