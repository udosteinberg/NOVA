/*
 * Checksum Functions
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

class Checksum
{
    public:
        /*
         * Compute additive checksum
         *
         * @param ptr   Pointer to array of T values
         * @param n     Number of T values to checksum
         * @return      Sum of T values, trimmed to T
         */
        template <typename T>
        static T additive (T const *ptr, unsigned n)
        {
            T val { 0 };

            while (n--)
                val += *ptr++;

            return val;
        }
};
