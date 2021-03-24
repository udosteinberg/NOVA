/*
 * Entry/Exit Functions
 *
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
 *
 */

#include "entry.hpp"

.section                .text
.balign                 0x800
.globl                  vector_table

vector_table:

// Current EL with EL2 using SP0, Synchronous
.org                    vector_table
                        b       .

// Current EL with EL2 using SP0, IRQ
.org                    vector_table + 0x80
                        b       .

// Current EL with EL2 using SP0, FIQ
.org                    vector_table + 0x100
                        b       .

// Current EL with EL2 using SP0, SError
.org                    vector_table + 0x180
                        b       .

// Current EL with EL2 using SPx, Synchronous
.org                    vector_table + 0x200
                        DECR_STACK
                        SAVE_STATE_EXC
                        mov     x0, sp
                        adr     lr, ret_kern_exception
                        b       exc_kern_handler

// Current EL with EL2 using SPx, IRQ
.org                    vector_table + 0x280
                        DECR_STACK
                        SAVE_STATE_IRQ
                        adr     lr, ret_kern_exception
                        b       irq_kern_handler

// Current EL with EL2 using SPx, FIQ
.org                    vector_table + 0x300
                        DECR_STACK
                        SAVE_STATE_IRQ
                        b       .

// Current EL with EL2 using SPx, SError
.org                    vector_table + 0x380
                        DECR_STACK
                        SAVE_STATE_IRQ
                        b       .

// Lower EL with EL1 using AArch64, Synchronous
.org                    vector_table + 0x400
                        SAVE_STATE_EXC
                        mov     x0, sp
                        adrp    x1, STCK_TOP
                        mov     sp, x1
                        b       exc_user_handler

// Lower EL with EL1 using AArch64, IRQ
.org                    vector_table + 0x480
                        SAVE_STATE_IRQ
                        adrp    x1, STCK_TOP
                        mov     sp, x1
                        b       irq_user_handler

// Lower EL with EL1 using AArch64, FIQ
.org                    vector_table + 0x500
                        SAVE_STATE_IRQ
                        b       .

// Lower EL with EL1 using AArch64, SError
.org                    vector_table + 0x580
                        SAVE_STATE_IRQ
                        b       .

// Lower EL with EL1 using AArch32, Synchronous
.org                    vector_table + 0x600
                        SAVE_STATE_EXC
                        mov     x0, sp
                        adrp    x1, STCK_TOP
                        mov     sp, x1
                        b       exc_user_handler

// Lower EL with EL1 using AArch32, IRQ
.org                    vector_table + 0x680
                        SAVE_STATE_IRQ
                        adrp    x1, STCK_TOP
                        mov     sp, x1
                        b       irq_user_handler

// Lower EL with EL1 using AArch32, FIQ
.org                    vector_table + 0x700
                        SAVE_STATE_IRQ
                        b       .

// Lower EL with EL1 using AArch32, SError
.org                    vector_table + 0x780
                        SAVE_STATE_IRQ
                        b       .

.org                    vector_table + 0x800
ret_kern_exception:     LOAD_STATE
                        INCR_STACK
                        eret
                        dsb     nsh
                        isb
