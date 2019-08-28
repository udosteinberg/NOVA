/*
 * Ticket Spinlock
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "types.hpp"

class Spinlock
{
    private:
        uint32 val { 0 };       // next[31:16] current[15:0]

    public:
        ALWAYS_INLINE
        inline void lock()
        {
            uint32 tmp;
            asm volatile ("prfm pstl1strm,   %0                 ;"
                          "1:   ldaxr   %w1, %0                 ;"
                          "     add     %w2, %w1, %4            ;"
                          "     stxr    %w3, %w2, %0            ;"
                          "     cbnz    %w3, 1b                 ;"
                          "     eor     %w3, %w1, %w1, ror #16  ;"
                          "     cbz     %w3, 2f                 ;"
                          "     sevl                            ;"
                          "1:   wfe                             ;"
                          "     ldaxrh  %w2, %0                 ;"
                          "     eor     %w3, %w2, %w1, lsr #16  ;"
                          "     cbnz    %w3, 1b                 ;"
                          "2:                                   ;"
                          : "+Q" (val), "=&r" (tmp), "=&r" (tmp), "=&r" (tmp) : "I" (0x10000) : "memory");
        }

        ALWAYS_INLINE
        inline void unlock()
        {
            asm volatile ("stlrh %w1, %0" : "+Q" (val) : "r" (val + 1) : "memory");
        }
};
