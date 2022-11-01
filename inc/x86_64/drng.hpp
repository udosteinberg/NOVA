/*
 * Digital Random Number Generator (DRNG)
 *
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

#include "cpu.hpp"

class Drng final
{
    private:
        /*
         * Obtain hardware-generated cryptographically secure random value
         */
        [[nodiscard]] static bool rand (uint64_t &v)
        {
            unsigned retries { 16 };

            bool r;
            do asm volatile ("rdrand %0" : "=r" (v), "=@ccc" (r) : : "cc"); while (!r && --retries);
            return r;
        }

        /*
         * Obtain hardware-generated nondeterministic random seed value
         */
        [[nodiscard]] static bool seed (uint64_t &v)
        {
            unsigned retries { 16 };

            bool r;
            do asm volatile ("rdseed %0" : "=r" (v), "=@ccc" (r) : : "cc"); while (!r && --retries);
            return r;
        }

    public:
        static bool rand (uint64_t (&v)[], unsigned const n)
        {
            if (EXPECT_FALSE (!Cpu::feature (Cpu::Feature::RDRAND)))
                return false;

            for (unsigned i { 0 }; i < n; i++)
                if (EXPECT_FALSE (!rand (v[i])))
                    return false;

            return true;
        }

        static bool seed (uint64_t (&v)[], unsigned const n)
        {
            if (EXPECT_FALSE (!Cpu::feature (Cpu::Feature::RDSEED)))
                return false;

            for (unsigned i { 0 }; i < n; i++)
                if (EXPECT_FALSE (!seed (v[i])))
                    return false;

            return true;
        }
};
