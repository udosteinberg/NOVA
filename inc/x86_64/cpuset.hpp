/*
 * CPU Set
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "assert.hpp"
#include "atomic.hpp"
#include "config.hpp"
#include "macros.hpp"
#include "types.hpp"

class Cpuset final
{
    private:
        static constexpr auto bits { 8 * sizeof (uintptr_t) };
        static constexpr auto elem { ((NUM_CPU + bits - 1) & ~(bits - 1)) / bits };

        static auto cpu_to_bit (cpu_t c) { return BITN (c % bits); }

        auto &cpu_to_msk (cpu_t c)       { return msk[c / bits]; }
        auto &cpu_to_msk (cpu_t c) const { return msk[c / bits]; }

        Atomic<uintptr_t> msk[elem] { 0 };

    public:
        ALWAYS_INLINE
        bool tst (cpu_t c) const
        {
            assert (c < NUM_CPU);

            return cpu_to_msk (c) & cpu_to_bit (c);
        }

        ALWAYS_INLINE
        void clr (cpu_t c)
        {
            assert (c < NUM_CPU);

            cpu_to_msk (c) &= ~cpu_to_bit (c);
        }

        ALWAYS_INLINE
        bool tas (cpu_t c)
        {
            assert (c < NUM_CPU);

            return cpu_to_msk (c).test_and_set (cpu_to_bit (c));
        }

        ALWAYS_INLINE
        void set()
        {
            for (unsigned i { 0 }; i < elem; i++)
                msk[i] = ~0UL;
        }
};
