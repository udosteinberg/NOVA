/*
 * Ticket Spinlock
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "macros.hpp"
#include "types.hpp"

class Spinlock final
{
    private:
        uint32_t val { 0 };     // nxt[31:16] cur[15:0]

    public:
        ALWAYS_INLINE
        inline void lock()
        {
            uint32_t tmp;

            asm volatile ("prfm pstl1strm,   %0                 ;"  // Prefetch lock into the cache
                          "1:   ldaxr   %w1, %0                 ;"  // %1 = old = val
                          "     add     %w2, %w1, %4            ;"  // %2 = val.nxt+1
                          "     stxr    %w3, %w2, %0            ;"  // %3 = update val with %2
                          "     cbnz    %w3, 1b                 ;"  // Someone beat us to the update
                          "     eor     %w3, %w1, %w1, ror #16  ;"  // Is the current ticket ours?
                          "     cbz     %w3, 2f                 ;"  // Enter critical section
                          "     sevl                            ;"  // Cancel the first WFE
                          "1:   wfe                             ;"  // Wait for unlock event
                          "     ldaxrh  %w2, %0                 ;"  // %2 = val.cur
                          "     eor     %w3, %w2, %w1, lsr #16  ;"  // Is the current ticket ours?
                          "     cbnz    %w3, 1b                 ;"  // Retry
                          "2:                                   ;"  // Critical section
                          : "+Q" (val), "=&r" (tmp), "=&r" (tmp), "=&r" (tmp) : "I" (BIT (16)) : "memory", "cc");
        }

        ALWAYS_INLINE
        inline void unlock()
        {
            asm volatile ("stlrh %w1, %0" : "+Q" (val) : "r" (val + 1) : "memory");
        }

        Spinlock() = default;

        Spinlock            (Spinlock const &) = delete;
        Spinlock& operator= (Spinlock const &) = delete;
};
