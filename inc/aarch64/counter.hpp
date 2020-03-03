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

#include "atomic.hpp"
#include "compiler.hpp"
#include "intid.hpp"
#include "memory.hpp"
#include "types.hpp"

class Counter
{
    private:
        static unsigned sgi[Intid::SGI_NUM] CPULOCAL;
        static unsigned ppi[Intid::PPI_NUM] CPULOCAL;

    public:
        static inline void count_sgi (unsigned i)
        {
            Atomic::store (sgi[i], 1 + Atomic::load (sgi[i], __ATOMIC_RELAXED), __ATOMIC_RELAXED);
        }

        static inline void count_ppi (unsigned i)
        {
            Atomic::store (ppi[i], 1 + Atomic::load (ppi[i], __ATOMIC_RELAXED), __ATOMIC_RELAXED);
        }

        static inline auto remote_sgi_count (unsigned cpu, unsigned i)
        {
            return Atomic::load (*reinterpret_cast<unsigned *>(reinterpret_cast<mword>(sgi + i) - CPU_LOCAL_DATA + CPU_GLOBL_DATA + cpu * PAGE_SIZE), __ATOMIC_RELAXED);
        }
};
