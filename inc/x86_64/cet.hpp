/*
 * Control-Flow Enforcement Technology (CET)
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

#include "compiler.hpp"
#include "extern.hpp"
#include "patch.hpp"
#include "types.hpp"

class Cet final
{
    public:
        /*
         * Deactivate supervisor shadow stack by marking the token as not busy
         */
        ALWAYS_INLINE
        static inline void sss_deactivate()
        {
#if defined(__CET__) && (__CET__ & 2)
            #define ASM_CET_1 clrssbsy %0;
            asm volatile (EXPAND (PATCH (ASM_CET_1,,PATCH_CET_SSS)) : : "m" (SSTK_TOP));
#endif
        }

        /*
         * Unwind supervisor shadow stack up to the token
         */
        ALWAYS_INLINE
        static inline void sss_unwind()
        {
#if defined(__CET__) && (__CET__ & 2)
            uintptr_t ssp;
            #define ASM_CET_2 rdsspq %0; sub %0, %1; shr $3, %1; incsspq %1;
            asm volatile (EXPAND (PATCH (ASM_CET_2,,PATCH_CET_SSS)) : "=&r" (ssp) : "r" (&SSTK_TOP));
#endif
        }
};
