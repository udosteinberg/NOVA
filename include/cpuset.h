/*
 * CPU Set
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

#include "atomic.h"
#include "compiler.h"
#include "types.h"

class Cpuset
{
    private:
        mword val;

    public:
        ALWAYS_INLINE
        inline explicit Cpuset() : val (0) {}

        ALWAYS_INLINE
        inline bool chk (unsigned cpu) const { return val & 1UL << cpu; }

        ALWAYS_INLINE
        inline bool set (unsigned cpu) { return !Atomic::test_set_bit<true>(val, cpu); }

        ALWAYS_INLINE
        inline void clr (unsigned cpu) { Atomic::clr_mask<true>(val, 1UL << cpu); }

        ALWAYS_INLINE
        inline void merge (Cpuset &s) { Atomic::set_mask<true>(val, s.val); }
};
