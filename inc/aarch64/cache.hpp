/*
 * Cache Maintenance
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "util.hpp"

class Cache final
{
    private:
        static inline constinit unsigned d_line_size { 64 };
        static inline constinit unsigned i_line_size { 64 };

    public:
        /*
         * Reduce system-wide cacheline size
         *
         * @param d     CPU D-Cache line size (must be a power of 2)
         * @param i     CPU I-Cache line size (must be a power of 2)
         */
        static inline void reduce (unsigned d, unsigned i)
        {
            if (is_power_of_2 (d)) [[likely]]
                d_line_size = min (d_line_size, d);

            if (is_power_of_2 (i)) [[likely]]
                i_line_size = min (i_line_size, i);
        }

        static inline void data_clean()
        {
            // FIXME: Unimplemented
        }

        static inline void data_clean (void const *ptr)
        {
            asm volatile ("dc cvac, %0; dsb sy" : : "r" (ptr) : "memory");
        }

        static inline void data_clean (void const *ptr, size_t size)
        {
            // Must be uintptr_t to retain all bits during negation
            uintptr_t const l { d_line_size };

            // Handle regions that split across cache lines
            auto s { reinterpret_cast<uintptr_t>(ptr) & ~(l - 1) };
            auto e { reinterpret_cast<uintptr_t>(ptr) + size };

            asm volatile ("1:   dc    cvac, %0      ;"
                          "     add     %0, %0, %2  ;"
                          "     cmp     %0, %1      ;"
                          "     blo     1b          ;"
                          "     dsb     sy          ;"
                          : "+&r" (s) : "r" (e), "r" (l) : "memory");
        }

        static inline void inst_invalidate()
        {
            asm volatile ("     ic      iallu       ;"  // Invalidate entire I$ to PoU
                          "     dsb     nsh         ;"  // Ensure cache invalidation completed
                          "     isb                 ;"  // Ensure fetched instructions use new cache state
                          : : : "memory");
        }
};
