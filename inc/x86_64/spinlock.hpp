/*
 * Ticket Spinlock
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.hpp"
#include "macros.hpp"
#include "types.hpp"

class Spinlock final
{
    private:
        uint32 val { 0 };       // nxt[31:16] cur[15:0]

    public:
        ALWAYS_INLINE
        inline void lock()
        {
            uint32 tkt = BIT (16), cur;

            asm volatile ("lock xadd    %k1, %0     ;"   // %1 = old = val; val.nxt++
                          "     movzwl  %w1, %2     ;"   // %2 = old.cur
                          "     shr     $16, %1     ;"   // %1 = old.nxt (our ticket)
                          "1:   cmp     %k1, %2     ;"   // Is the current ticket ours?
                          "     je      2f          ;"   // Enter critical section
                          "     pause               ;"   // Spin loop hint
                          "     movzwl  %w0, %2     ;"   // %2 = val.cur
                          "     jmp     1b          ;"   // Retry
                          "2:                       ;"   // Critical section
                          : "+m" (val), "+r" (tkt), "=&r" (cur) : : "memory", "cc");
        }

        ALWAYS_INLINE
        inline void unlock()
        {
            asm volatile ("incw %0" : "+m" (val) : : "memory", "cc");
        }

        Spinlock() = default;

        Spinlock            (Spinlock const &) = delete;
        Spinlock& operator= (Spinlock const &) = delete;
};
