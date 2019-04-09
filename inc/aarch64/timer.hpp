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
        static inline uint32    freq    { 0 };  // Frequency in Hz
        static inline uint64    offs    { 0 };  // Skew between physical and system time

        /*
         * Get absolute physical time
         *
         * @return      Current absolute physical time
         */
        ALWAYS_INLINE
        static inline uint64 pct()
        {
            uint64 p;
            asm volatile ("isb; mrs %0, cntpct_el0" : "=r" (p) : : "memory");
            return p;
        }

    public:
        static inline unsigned  ppi_el2_p   { Board::tmr[0].ppi };
        static inline unsigned  ppi_el1_v   { Board::tmr[1].ppi };
        static inline bool      lvl_el2_p   { !!(Board::tmr[0].flg & BIT_RANGE (3, 2)) };
        static inline bool      lvl_el1_v   { !!(Board::tmr[1].flg & BIT_RANGE (3, 2)) };

        /*
         * Convert absolute physical time to absolute system time
         *
         * @param p     Absolute physical time
         * @return      Absolute system time
         */
        ALWAYS_INLINE
        static inline uint64 phys_to_syst (uint64 p) { return p - offs; }

        /*
         * Convert absolute system time to absolute physical time
         *
         * @param s     Absolute system time
         * @return      Absolute physical time
         */
        ALWAYS_INLINE
        static inline uint64 syst_to_phys (uint64 s) { return s + offs; }

        /*
         * Get absolute system time
         *
         * @return      Current absolute system time
         */
        ALWAYS_INLINE
        static inline uint64 time() { return phys_to_syst (pct()); }

        /*
         * Set absolute system time deadline
         *
         * @param s     Absolute system time deadline
         */
        ALWAYS_INLINE
        static inline void set_dln (uint64 s)
        {
            asm volatile ("msr cnthp_cval_el2, %0; isb" : : "r" (syst_to_phys (s)));
        }

        /*
         * Cancel deadline
         */
        ALWAYS_INLINE
        static inline void stop()
        {
            set_dln (~0ULL);
        }

        /*
         * Convert relative system time to relative wall clock time
         *
         * @param ticks Relative system time in ticks
         * @return      Relative wall clock time in ms
         */
        ALWAYS_INLINE
        static inline auto ticks_to_ms (uint64 ticks)
        {
            return ticks * 1000 / freq;
        }

        /*
         * Convert relative wall clock time to relative system time
         *
         * @param ms    Relative wall clock time in ms
         * @return      Relative system time in ticks
         */
        ALWAYS_INLINE
        static inline auto ms_to_ticks (uint32 ms)
        {
            // Note: ms must be non-zero and freq must be >= 1kHz
            return static_cast<uint64>(ms) * freq / 1000;
        }

        static void interrupt();
        static void init();
};
