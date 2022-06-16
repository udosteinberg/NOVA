/*
 * CPU Set
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "atomic.hpp"
#include "macros.hpp"
#include "types.hpp"

class Cpuset final
{
    private:
        Atomic<uintptr_t> msk[1] { 0 };

        static constexpr auto bits { 8 * sizeof (*msk) };

        inline auto &cpu_to_msk (unsigned c)       { return msk[c / bits]; }
        inline auto &cpu_to_msk (unsigned c) const { return msk[c / bits]; }

        static inline auto cpu_to_bit (unsigned c) { return BITN (c % bits); }

    public:
        ALWAYS_INLINE
        inline bool tst (unsigned c) const { return cpu_to_msk (c) & cpu_to_bit (c); }

        ALWAYS_INLINE
        inline void clr (unsigned c) { cpu_to_msk (c) &= ~cpu_to_bit (c); }

        ALWAYS_INLINE
        inline bool tas (unsigned c) { return cpu_to_msk (c).test_and_set (cpu_to_bit (c)); }

        ALWAYS_INLINE
        inline void set()
        {
            for (unsigned i = 0; i < sizeof (msk) / sizeof (*msk); i++)
                msk[i] = ~0UL;
        }
};
