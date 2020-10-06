/*
 * Multiboot Headers
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "alignment.hpp"
#include "macros.hpp"
#include "multiboot.hpp"

.section .head
                        .balign 8

.Lmb2:                  .long   MULTIBOOT_V2_HEADER                     // Magic
                        .long   0                                       // Architecture
                        .long   .Lmb1 - .Lmb2                           // Header Length
                        .long -(.Lmb1 - .Lmb2) - MULTIBOOT_V2_HEADER    // Checksum
                        .quad   0x0000000800000006                      // Tag: Module Alignment
                        .quad   0x0000000800000007                      // Tag: EFI Boot Services
                        .quad   0x000000180000000a                      // Tag: Relocatable Header
                        .long   ALIGNMENT_ADDR (ALIGNMENT_NOVA)         //      Min Addr
                        .long   BIT_RANGE (31, 0)                       //      Max Addr
                        .long   ALIGNMENT_ADDR (ALIGNMENT_NOVA)         //      Alignment
                        .long   0x2                                     //      Preference
                        .quad   0x0000000800000000                      // Tag: End

.Lmb1:                  .long   MULTIBOOT_V1_HEADER                     // Magic
                        .long   BIT (0)                                 // Flags
                        .long  -BIT (0) - MULTIBOOT_V1_HEADER           // Checksum
