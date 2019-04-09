/*
 * Timer: Architecture-Specific (ARM)
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

#include "board.hpp"
#include "macros.hpp"
#include "stc.hpp"

class Timer final : private Stc
{
    private:
        static inline uint64_t offs { 0 };  // Skew between physical and system time

        /*
         * Get absolute physical time
         *
         * @return      Current absolute physical time
         */
        ALWAYS_INLINE
        static inline auto pct()
        {
            uint64_t val;
            asm volatile ("isb; mrs %x0, cntpct_el0" : "=r" (val) : : "memory");
            return val;
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
        static inline auto phys_to_syst (uint64_t p) { return p - offs; }

        /*
         * Convert absolute system time to absolute physical time
         *
         * @param s     Absolute system time
         * @return      Absolute physical time
         */
        ALWAYS_INLINE
        static inline auto syst_to_phys (uint64_t s) { return s + offs; }

        /*
         * Get absolute system time
         *
         * @return      Current absolute system time
         */
        ALWAYS_INLINE
        static inline auto time() { return phys_to_syst (pct()); }

        /*
         * Set absolute system time deadline
         *
         * @param s     Absolute system time deadline
         */
        ALWAYS_INLINE
        static inline void set_dln (uint64_t s)
        {
            asm volatile ("msr cnthp_cval_el2, %x0; isb" : : "rZ" (syst_to_phys (s)));
        }

        /*
         * Stop timer
         */
        ALWAYS_INLINE
        static inline void stop()
        {
            asm volatile ("msr cnthp_cval_el2, %x0; isb" : : "rZ" (~0ULL));
        }

        static void init();
};
