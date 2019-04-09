/*
 * Generic Timer
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "board.hpp"
#include "compiler.hpp"
#include "macros.hpp"
#include "types.hpp"

class Timer
{
    private:
        static uint32 freq CPULOCAL;    // Timer frequency in Hz

        enum Control : uint64
        {
            ENABLE      = BIT64 (0),    // Timer enabled
            IMASK       = BIT64 (1),    // Timer interrupt is masked
            ISTATUS     = BIT64 (2),    // Timer condition is met
        };

        static uint32 frequency()
        {
            uint32 val;
            asm volatile ("isb; mrs %x0, cntfrq_el0" : "=r" (val) : : "memory");
            return val;
        }

        static inline void set_access (uint64 val)
        {
            asm volatile ("msr cnthctl_el2, %0" : : "r" (val));
        }

        static inline void set_control (Control val)
        {
            asm volatile ("msr cnthp_ctl_el2, %0" : : "r" (val));
        }

    public:
        static inline unsigned  ppi_el2_p   { Board::tmr[0].ppi };
        static inline unsigned  ppi_el1_v   { Board::tmr[1].ppi };
        static inline bool      lvl_el2_p   { !!(Board::tmr[0].flg & BIT_RANGE (3, 2)) };
        static inline bool      lvl_el1_v   { !!(Board::tmr[1].flg & BIT_RANGE (3, 2)) };

        ALWAYS_INLINE
        static inline uint64 time()
        {
            uint64 val;
            asm volatile ("isb; mrs %0, cntpct_el0" : "=r" (val) : : "memory");
            return val;
        }

        ALWAYS_INLINE
        static inline void set_dln (uint64 ticks)
        {
            asm volatile ("msr cnthp_cval_el2, %0; isb" : : "r" (ticks));
        }

        ALWAYS_INLINE
        static inline void stop()
        {
            set_dln (~0ULL);
        }

        ALWAYS_INLINE
        static inline auto ticks_to_ms (uint64 ticks)
        {
            return ticks * 1000 / freq;
        }

        ALWAYS_INLINE
        static inline auto ms_to_ticks (uint32 ms)
        {
            // Note: ms must be non-zero and freq must be >= 1kHz
            return static_cast<uint64>(ms) * freq / 1000;
        }

        static void interrupt();
        static void init();
};
