/*
 * User Memory Access
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "arch.hpp"
#include "compiler.hpp"

class User
{
    public:
        template <typename T>
        ALWAYS_INLINE
        static inline mword peek (T *addr, T &val)
        {
            mword ret;
            asm volatile ("1: mov %2, %1; or $-1, %0; 2:"
                          ".section .fixup,\"a\"; .align 8;" EXPAND (WORD) " 1b,2b; .previous"
                          : "=a" (ret), "=q" (val) : "m" (*addr));
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline mword cmp_swap (T *addr, T o, T n)
        {
            mword ret;
            asm volatile ("1: lock; cmpxchg %3, %1; or $-1, %0; 2:"
                          ".section .fixup,\"a\"; .align 8;" EXPAND (WORD) " 1b,2b; .previous"
                          : "=a" (ret), "+m" (*addr) : "a" (o), "q" (n));
            return ret;
        }
};
