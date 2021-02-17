/*
 * CPU Set
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

#include "types.hpp"

class Cpuset
{
    private:
        mword val;

        template <typename T>
        static inline bool test_set_bit (T &val, unsigned long bit)
        {
            bool ret;
            asm volatile ("lock; bts%z1 %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }

    public:
        ALWAYS_INLINE
        inline explicit Cpuset() : val (0) {}

        ALWAYS_INLINE
        inline bool chk (unsigned cpu) const { return val & 1UL << cpu; }

        ALWAYS_INLINE
        inline bool set (unsigned cpu) { return !test_set_bit (val, cpu); }

        ALWAYS_INLINE
        inline void clr (unsigned cpu) { __atomic_and_fetch (&val, ~(1UL << cpu), __ATOMIC_SEQ_CST); }

        ALWAYS_INLINE
        inline void merge (Cpuset &s) { __atomic_or_fetch (&val, s.val, __ATOMIC_SEQ_CST); }
};
