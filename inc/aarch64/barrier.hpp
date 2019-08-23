/*
 * Barriers
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

#include "std.hpp"

class Barrier final
{
    public:
        enum class Domain
        {
            OSH = 0,    // Outer Shareable
            NSH = 1,    // Non-Shareable
            ISH = 2,    // Inner Shareable
            SY  = 3,    // Full System
        };

        enum class Type
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
        static void rmb (Domain d) { dmb (d, Type::LD); }

        /*
         * Write Data Memory Barrier
         *
         * LD ---|-->
         * ST -->|
         *    <--|--- LD
         *       |<-- ST
         */
        static void wmb (Domain d) { dmb (d, Type::ST); }

        /*
         * Full Data Memory Barrier
         *
         * LD -->|
         * ST -->|
         *       |<-- LD
         *       |<-- ST
         */
        static void fmb (Domain d) { dmb (d, Type::ALL); }

        /*
         * Read Data Synchronization Barrier
         *
         * Like rmb, but additionally ensures completion of LD memory accesses before the barrier
         */
        static void rsb (Domain d) { dsb (d, Type::LD); }

        /*
         * Write Data Synchronization Barrier
         *
         * Like wmb, but additionally ensures completion of ST memory accesses before the barrier
         */
        static void wsb (Domain d) { dsb (d, Type::ST); }

        /*
         * Full Data Synchronization Barrier
         *
         * Like fmb, but additionally ensures completion of ALL memory accesses before the barrier
         */
        static void fsb (Domain d) { dsb (d, Type::ALL); }

        /*
         * Instruction Synchronization Barrier
         */
        static void isb() { asm volatile ("isb" : : : "memory"); }

    private:
        /*
         * Data Memory Barrier
         */
        static void dmb (Domain d, Type t) { asm volatile ("dmb #%0" : : "i" (std::to_underlying (d) << 2 | std::to_underlying (t)) : "memory"); }

        /*
         * Data Synchronization Barrier
         */
        static void dsb (Domain d, Type t) { asm volatile ("dsb #%0" : : "i" (std::to_underlying (d) << 2 | std::to_underlying (t)) : "memory"); }
};
