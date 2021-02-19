/*
 * Cache Maintenance
 *
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

#include "compiler.hpp"
#include "types.hpp"

class Cache
{
    private:
        static unsigned dcache_line_size CPULOCAL;
        static unsigned icache_line_size CPULOCAL;

    public:
        ALWAYS_INLINE
        static inline void init (unsigned s)
        {
            dcache_line_size = icache_line_size = s;
        }

        ALWAYS_INLINE
        static inline void data_clean()
        {
            asm volatile ("wbinvd" : : : "memory");
        }

        ALWAYS_INLINE
        static inline void data_clean (void const *ptr)
        {
            asm volatile ("clflush (%0)" : : "r" (ptr) : "memory");
        }

        ALWAYS_INLINE
        static inline void data_clean (void const *ptr, size_t size)
        {
            // Assumes if size is less than dcache_line_size, the region is NOT split across cache lines
            asm volatile ("1:   clflush (%0)    ;"
                          "     add     %2, %0  ;"
                          "     cmp     %1, %0  ;"
                          "     jb      1b      ;"
                          : "+&r" (ptr) : "r" (static_cast<char const *>(ptr) + size), "i" (64) : "memory");

            // FIXME: Currently using 64 instead of dcache_line_size, because we need this function in
            // early bootstrap when CPULOCAL is not yet available.
        }
};
