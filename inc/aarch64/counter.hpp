/*
 * Event Counters
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
#include "intid.hpp"
#include "memory.hpp"
#include "types.hpp"

class Counter
{
    private:
        unsigned val;

    public:
        static Counter req[Intid::NUM_SGI]  CPULOCAL;
        static Counter loc[Intid::NUM_PPI]  CPULOCAL;
        static Counter schedule             CPULOCAL;
        static Counter helping              CPULOCAL;

        inline void inc()
        {
            __atomic_store_n (&val, val + 1, __ATOMIC_RELAXED);
        }

        inline auto get (unsigned cpu) const
        {
            return __atomic_load_n (reinterpret_cast<decltype (val) *>(reinterpret_cast<uintptr_t>(&val) - CPU_LOCAL_DATA + CPU_GLOBL_DATA + cpu * PAGE_SIZE), __ATOMIC_RELAXED);
        }
};
