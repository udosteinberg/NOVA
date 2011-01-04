/*
 * Generic Spinlock
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.h"
#include "types.h"

class Spinlock
{
    private:
        uint16 val;

    public:
        ALWAYS_INLINE
        inline Spinlock() : val (0) {}

        NOINLINE
        void lock()
        {
            uint16 tmp = 0x100;

            asm volatile ("     lock; xadd %0, %1;  "
                          "1:   cmpb %h0, %b0;      "
                          "     je 2f;              "
                          "     pause;              "
                          "     movb %1, %b0;       "
                          "     jmp 1b;             "
                          "2:                       "
                          : "+Q" (tmp), "+m" (val) : : "memory");
        }

        ALWAYS_INLINE
        inline void unlock()
        {
            asm volatile ("incb %0" : "=m" (val) : : "memory");
        }
};
