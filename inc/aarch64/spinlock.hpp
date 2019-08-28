/*
 * Generic Spinlock
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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
        uint32 val { 0 };

    public:
        ALWAYS_INLINE
        void lock()
        {
            mword dummy;
            asm volatile ("prfm pstl1keep,   %0;        "
                          "1:   ldaxr   %w1, %0;        "
                          "     cbnz    %w1, 1b;        "
                          "     stxr    %w1, %w2, %0;   "
                          "     cbnz    %w1, 1b;        "
                          : "+Q" (val), "=&r" (dummy) : "r" (1) : "memory");
        }

        ALWAYS_INLINE
        void unlock()
        {
            asm volatile ("     stlr    wzr, %0;        "
                          : "+Q" (val) : : "memory");
        }
};
