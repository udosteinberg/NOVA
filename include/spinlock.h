/*
 * Generic Spinlock
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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
        uint8 val;

    public:
        ALWAYS_INLINE
        inline Spinlock() : val (1) {}

        ALWAYS_INLINE
        inline void lock()
        {
            asm volatile ("1:   lock; decb %0;  "
                          "     jns 3f;         "
                          "2:   pause;          "
                          "     cmpb $0, %0;    "
                          "     jle 2b;         "
                          "     jmp 1b;         "
                          "3:                   "
                          : "+m" (val) : : "memory");
        }

        ALWAYS_INLINE
        inline void unlock()
        {
            asm volatile ("movb $1, %0" : "=m" (val) : : "memory");
        }
};
