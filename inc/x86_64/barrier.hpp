/*
 * Barriers
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.hpp"

class Barrier final
{
    public:
        /*
         * Read Memory Barrier
         *
         *      LD -->|         |<-- LD
         *      ST ---|-->+     |<-- ST
         * CLFLUSH -->|         |<-- CLFLUSH
         *
         * Additional serialization
         * - all prior instructions must complete before LFENCE executes
         * - all later instructions cannot execute until LFENCE completes (not even speculatively)
         * + for prior store instructions, stored data may become globally visible after LFENCE
         */
        ALWAYS_INLINE static inline void rmb() { asm volatile ("lfence" : : : "memory"); }

        /*
         * Write Memory Barrier
         *
         *      LD ---|-->   <--|--- LD
         *      ST -->|         |<-- ST
         * CLFLUSH -->|         |<-- CLFLUSH
         */
        ALWAYS_INLINE static inline void wmb() { asm volatile ("sfence" : : : "memory"); }

        /*
         * Full Memory Barrier
         *
         *      LD -->|         |<-- LD
         *      ST -->|         |<-- ST
         * CLFLUSH -->|         |<-- CLFLUSH
         */
        ALWAYS_INLINE static inline void fmb() { asm volatile ("mfence" : : : "memory"); }
};

ALWAYS_INLINE
inline void barrier()
{
    asm volatile ("" : : : "memory");
}
