/*
 * MLE Header
 *
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

.global __head_mle

.section .head
                        .balign 8

__head_mle:             .octa   0x42b651cba2555c0f74a7476f9082ac5a  //  0   UUID
                        .long   .Lhead_mle - __head_mle             // 16   Length
                        .long   0x20003                             // 20   Version
                        .long   HASH_INIT                           // 24   ILP Entry
                        .long   0                                   // 28   First Page
                        .long   0                                   // 32   MLE Start
                        .long   HASH_SIZE                           // 36   MLE End
                        .long   BIT_RANGE (10, 0)                   // 40   MLE Caps
                        .long   0                                   // 44   Cmd Start
                        .long   0                                   // 48   Cmd End
.Lhead_mle:
