/*
 * Multiboot Headers
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "macros.hpp"
#include "multiboot.hpp"

.section .head
                        .balign 8

.Lmb2:                  .long   MULTIBOOT_V2_HEADER
                        .long   0
                        .long   .Lmb1 - .Lmb2
                        .long -(.Lmb1 - .Lmb2) - MULTIBOOT_V2_HEADER
                        .quad   0x0000000800000006
                        .quad   0x0000000800000007
                        .quad   0x000000180000000a
                        .quad   0xffffffff00200000
                        .quad   0x0000000200200000
                        .quad   0x0000000800000000

.Lmb1:                  .long   MULTIBOOT_V1_HEADER
                        .long   BIT (0)
                        .long  -BIT (0) - MULTIBOOT_V1_HEADER
