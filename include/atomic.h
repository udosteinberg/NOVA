/*
 * Atomic Operations
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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
        static inline void add (T &val, T n)
        {
            asm volatile ("lock; add %1, %0" : "+m" (val) : "ir" (n) : "cc");
        }

        template <typename T>
        ALWAYS_INLINE
        static inline bool sub (T &val, T n)
        {
            bool ret;
            asm volatile ("lock; sub %2, %1; setz %0" : "=q" (ret), "+m" (val) : "ir" (n) : "cc");
            return ret;
        }

        template <bool L, typename T>
        ALWAYS_INLINE
        static inline bool test_clr_bit (T &val, unsigned long bit)
        {
            bool ret;
            if (L)
                asm volatile ("lock; btr %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            else
                asm volatile ("      btr %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }

        template <bool L, typename T>
        ALWAYS_INLINE
        static inline bool test_set_bit (T &val, unsigned long bit)
        {
            bool ret;
            if (L)
                asm volatile ("lock; bts %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            else
                asm volatile ("      bts %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }

        template <bool L, typename T>
        ALWAYS_INLINE
        static inline void set_mask (T &val, unsigned long mask)
        {
            if (L)
                asm volatile ("lock; or%z0 %1, %0" : "+m" (val) : "ir" (mask) : "cc");
            else
                asm volatile ("      or%z0 %1, %0" : "+m" (val) : "ir" (mask) : "cc");
        }

        template <bool L, typename T>
        ALWAYS_INLINE
        static inline void clr_mask (T &val, unsigned long mask)
        {
            if (L)
                asm volatile ("lock; and%z0 %1, %0" : "+m" (val) : "ir" (~mask) : "cc");
            else
                asm volatile ("      and%z0 %1, %0" : "+m" (val) : "ir" (~mask) : "cc");
        }

        template <bool L, typename T>
        ALWAYS_INLINE
        static inline bool cmp_swap (T *ptr, T o, T n)
        {
            bool ret;
            if (L)
                asm volatile ("lock; cmpxchg %3, %1; sete %0" : "=q" (ret), "+m" (*ptr), "+a" (o) : "r" (n) : "cc");
            else
                asm volatile ("      cmpxchg %3, %1; sete %0" : "=q" (ret), "+m" (*ptr), "+a" (o) : "r" (n) : "cc");
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline T exchange (T *ptr, T n)
        {
            asm volatile ("xchg %1, %0" : "+m" (*ptr), "+r" (n) : : "cc");
            return n;
        }
};
