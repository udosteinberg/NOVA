/*
 * Instruction Patching
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

#pragma once

#include "patch_arch.hpp"

/*
 * Portability note: Some x86 assemblers (e.g., on MacOS) don't recognize
 * '/' as division operator by default and instead treat it as comment.
 * Passing --divide on the assembler command line fixes that behavior.
 */
#define PATCH(O, N, T)  .set F, (904f - 903f) - (901f - 900f);          \
900:                    O;                                              \
901:                    .fill -(F > 0) * F / NOP_LEN, NOP_LEN, NOP_OPC; \
902:                    .pushsection .patch.code, "a";                  \
903:                    N;                                              \
904:                    .popsection;                                    \
                        .pushsection .patch.data, "a";                  \
                        .balign 4;                                      \
905:                    .long 900b - 905b;                              \
                        .long 903b - 905b;                              \
                        .byte 902b - 900b;                              \
                        .byte 904b - 903b;                              \
                        .word T;                                        \
                        .popsection;

#ifndef __ASSEMBLER__

#include "types.hpp"

class Patch final
{
    private:
        int32_t     off_old;
        int32_t     off_new;
        uint8_t     len_old;
        uint8_t     len_new;
        uint16_t    tag;

    public:
        static void init();
};

#endif
