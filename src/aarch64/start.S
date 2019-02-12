/*
 * Startup Code
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "memory.hpp"

.global                 __init_bsp

/*
 * Initialization Code
 */
.section .init

__init_bsp:
                        // Determine relocated addresses
                        adrp    x25, NOVA_HPAS
                        adrp    x26, PTAB_HPAS
                        adrp    x27, ZERO_HPAS
                        adrp    x28, NOVA_HPAE
                        mov     x29, LINK_ADDR
                        sub     x29, x29, x25

                        // Check load address alignment
                        ubfx    x2, x25, #0, #ALIGNMENT_NOVA
                        cbz     x2, .Linit_all
.Ldead:
                        // Fail boot
                        ldr     lr, =0xd00d2badc0dedead
                        ret

.Linit_all:             msr     daifset, #0xf
                        msr     spsel,   #0x1

                        // Switch to boot stack
                        adr     x0, STACK
                        mov     sp, x0

                        // General initialization
                        bl      init

                        b       .
