/*
 * Atomic Operations
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

class Atomic
{
    public:
        template <typename T>
        ALWAYS_INLINE
        static inline bool cmp_swap (T &ptr, T o, T n) { return __sync_bool_compare_and_swap (&ptr, o, n); }

        template <typename T>
        ALWAYS_INLINE
        static inline T add (T &ptr, T v) { return __sync_add_and_fetch (&ptr, v); }

        template <typename T>
        ALWAYS_INLINE
        static inline T sub (T &ptr, T v) { return __sync_sub_and_fetch (&ptr, v); }

        template <typename T>
        ALWAYS_INLINE
        static inline void set_mask (T &ptr, T v) { __sync_or_and_fetch (&ptr, v); }

        template <typename T>
        ALWAYS_INLINE
        static inline void clr_mask (T &ptr, T v) { __sync_and_and_fetch (&ptr, ~v); }

        template <typename T>
        ALWAYS_INLINE
        static inline bool test_set_bit (T &val, unsigned long bit)
        {
            bool ret;
            asm volatile ("lock; bts%z1 %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline bool test_clr_bit (T &val, unsigned long bit)
        {
            bool ret;
            asm volatile ("lock; btr%z1 %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }
};
