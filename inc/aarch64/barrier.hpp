/*
 * Barriers
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

class Barrier final
{
    public:
        enum Domain
        {
            OSH = 0,    // Outer Shareable
            NSH = 1,    // Non-Shareable
            ISH = 2,    // Inner Shareable
            SY  = 3,    // Full System
        };

        enum Type
        {
            LD  = 1,    // Loads
            ST  = 2,    // Stores
            ALL = 3,    // Loads and Stores
        };

        /*
         * Read Data Memory Barrier
         *
         * LD -->|
         * ST ---|-->
         *       |<--LD
         *       |<--ST
         */
        ALWAYS_INLINE static inline void rmb (Domain d) { dmb (d, LD); }

        /*
         * Write Data Memory Barrier
         *
         * LD ---|-->
         * ST -->|
         *    <--|--- LD
         *       |<-- ST
         */
        ALWAYS_INLINE static inline void wmb (Domain d) { dmb (d, ST); }

        /*
         * Full Data Memory Barrier
         *
         * LD -->|
         * ST -->|
         *       |<-- LD
         *       |<-- ST
         */
        ALWAYS_INLINE static inline void fmb (Domain d) { dmb (d, ALL); }

        /*
         * Read Data Synchronization Barrier
         *
         * Like rmb, but additionally ensures completion of LD memory accesses before the barrier
         */
        ALWAYS_INLINE static inline void rsb (Domain d) { dsb (d, LD); }

        /*
         * Write Data Synchronization Barrier
         *
         * Like wmb, but additionally ensures completion of ST memory accesses before the barrier
         */
        ALWAYS_INLINE static inline void wsb (Domain d) { dsb (d, ST); }

        /*
         * Full Data Synchronization Barrier
         *
         * Like fmb, but additionally ensures completion of ALL memory accesses before the barrier
         */
        ALWAYS_INLINE static inline void fsb (Domain d) { dsb (d, ALL); }

        /*
         * Instruction Synchronization Barrier
         */
        ALWAYS_INLINE static inline void isb() { asm volatile ("isb" : : : "memory"); }

    private:
        /*
         * Data Memory Barrier
         */
        ALWAYS_INLINE static inline void dmb (Domain d, Type t) { asm volatile ("dmb #%0" : : "i" (d << 2 | t) : "memory"); }

        /*
         * Data Synchronization Barrier
         */
        ALWAYS_INLINE static inline void dsb (Domain d, Type t) { asm volatile ("dsb #%0" : : "i" (d << 2 | t) : "memory"); }
};
