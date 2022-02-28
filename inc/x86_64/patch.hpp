/*
 * Instruction Patching
 *
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

#pragma once

#define NOP_OPCODE      0x90

#define PATCH(O, N, T)  .set SKIP, (904f - 903f) - (901f - 900f);   \
900:                    O;                                          \
901:                    .skip -(SKIP > 0) * SKIP, NOP_OPCODE;       \
902:                    .pushsection .patch.code, "a";              \
903:                    N;                                          \
904:                    .popsection;                                \
                        .pushsection .patch.data, "a";              \
                        .balign 4;                                  \
905:                    .long 900b - 905b;                          \
                        .long 903b - 905b;                          \
                        .byte 902b - 900b;                          \
                        .byte 904b - 903b;                          \
                        .word T;                                    \
                        .popsection;

#define PATCH_XSAVES    0
#define PATCH_CET_SS    1
#define PATCH_CET_IBT   2

#ifndef __ASSEMBLER__

#include "types.hpp"

class Patch
{
    private:
        int32_t off_old;
        int32_t off_new;
        uint8   len_old;
        uint8   len_new;
        uint16  tag;

    public:
        static void init();
};

#endif
