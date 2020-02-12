/*
 * Cache Maintenance
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
    friend class Cpu;

    private:
        static unsigned dcache_line_size CPULOCAL;
        static unsigned icache_line_size CPULOCAL;

    public:
        ALWAYS_INLINE
        static inline void clean_dcache (void const *ptr)
        {
            asm volatile ("clflush (%0)" : : "r" (ptr) : "memory");
        }

        ALWAYS_INLINE
        static inline void clean_dcache (void const *ptr, size_t size)
        {
            asm volatile ("1:   clflush (%0)    ;"
                          "     add     %2, %0  ;"
                          "     cmp     %1, %0  ;"
                          "     jb      1b      ;"
                          : "+&r" (ptr) : "r" (static_cast<char const *>(ptr) + size), "r" (static_cast<size_t>(dcache_line_size)) : "memory");
        }
};
