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

/*
 * Entry from EL2 using SP0
 */
.org                    vector_table                // Synchronous
                        b       .

.org                    vector_table + 0x80         // IRQ
                        b       .

.org                    vector_table + 0x100        // FIQ
                        b       .

.org                    vector_table + 0x180        // SError
                        b       .

/*
 * Entry from EL2 using SPx
 */
.org                    vector_table + 0x200        // Synchronous
                        DECR_STACK
                        SAVE_STATE_EXC
                        b       1f

.org                    vector_table + 0x280        // IRQ
                        DECR_STACK
                        SAVE_STATE
                        b       1f

.org                    vector_table + 0x300        // FIQ
                        b       .

.org                    vector_table + 0x380        // SError
                        DECR_STACK
                        SAVE_STATE_EXC
                        b       .

/*
 * Entry from EL0/EL1 with EL1 using AArch64
 */
.org                    vector_table + 0x400        // Synchronous
                        SAVE_STATE_EXC
                        b       .

.org                    vector_table + 0x480        // IRQ
                        SAVE_STATE
                        b       .

.org                    vector_table + 0x500        // FIQ
                        b       .

.org                    vector_table + 0x580        // SError
                        SAVE_STATE_EXC
                        b       .

/*
 * Entry from EL0/EL1 with EL1 using AArch32
 */
.org                    vector_table + 0x600        // Synchronous
                        SAVE_STATE_EXC
                        b       .

.org                    vector_table + 0x680        // IRQ
                        SAVE_STATE
                        b       .

.org                    vector_table + 0x700        // FIQ
                        b       .

.org                    vector_table + 0x780        // SError
                        SAVE_STATE_EXC
                        b       .

/*
 * Exit to EL2
 */
.org                    vector_table + 0x800
1:                      LOAD_STATE
                        INCR_STACK
                        ERET
