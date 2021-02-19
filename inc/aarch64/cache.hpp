/*
 * Cache Maintenance
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

class Cache final
{
    private:
        static unsigned dcache_line_size CPULOCAL;
        static unsigned icache_line_size CPULOCAL;

    public:
        ALWAYS_INLINE
        static inline void init (unsigned d, unsigned i)
        {
            dcache_line_size = d;
            icache_line_size = i;
        }

        ALWAYS_INLINE
        static inline void data_clean()
        {
            // FIXME: Unimplemented
        }

        ALWAYS_INLINE
        static inline void data_clean (void const *ptr)
        {
            asm volatile ("dc cvac, %0; dsb sy" : : "r" (ptr) : "memory");
        }

        ALWAYS_INLINE
        static inline void data_clean (void const *ptr, size_t size)
        {
            // Assumes if size is less than dcache_line_size, the region is NOT split across cache lines
            asm volatile ("1:   dc    cvac, %0      ;"
                          "     add     %0, %0, %2  ;"
                          "     cmp     %0, %1      ;"
                          "     blo     1b          ;"
                          "     dsb     sy          ;"
                          : "+&r" (ptr) : "r" (static_cast<char const *>(ptr) + size), "r" (static_cast<size_t>(dcache_line_size)) : "memory");
        }

        ALWAYS_INLINE
        static inline void inst_invalidate()
        {
            asm volatile ("     ic      iallu       ;"  // Invalidate entire I$ to PoU
                          "     dsb     nsh         ;"  // Ensure cache invalidation completed
                          "     isb                 ;"  // Ensure fetched instructions use new cache state
                          : : : "memory");
        }
};
