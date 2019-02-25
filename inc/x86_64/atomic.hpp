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

#include "atomic_base.hpp"

class Atomic : public Atomic_base
{
    public:
        template <typename T>
        static inline bool test_set_bit (T &val, unsigned long bit)
        {
            bool ret;
            asm volatile ("lock; bts%z1 %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }

        template <typename T>
        static inline bool test_clr_bit (T &val, unsigned long bit)
        {
            bool ret;
            asm volatile ("lock; btr%z1 %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }
};
