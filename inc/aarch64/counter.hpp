/*
 * Event Counters
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

#include "atomic.hpp"
#include "compiler.hpp"
#include "intid.hpp"
#include "kmem.hpp"
#include "types.hpp"

class Counter final
{
    private:
        Atomic<unsigned> val { 0 };

    public:
        static Counter req[Intid::NUM_SGI]  CPULOCAL;
        static Counter loc[Intid::NUM_PPI]  CPULOCAL;
        static Counter schedule             CPULOCAL;
        static Counter helping              CPULOCAL;

        ALWAYS_INLINE
        inline void inc()
        {
            val = val + 1;
        }

        ALWAYS_INLINE
        inline unsigned get (unsigned cpu) const
        {
            return *Kmem::loc_to_glob (&val, cpu);
        }
};
