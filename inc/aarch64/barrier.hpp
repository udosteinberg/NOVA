/*
 * Barriers
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

class Barrier
{
    public:
        /*
         * Instruction Synchronization Barrier
         * Instructions ahead of the barrier must complete before the barrier.
         * Instructions beyond the barrier must wait for the barrier to complete.
         */
        ALWAYS_INLINE
        static inline void isb() { asm volatile ("isb" : : : "memory"); }

        /*
         * Full Memory Barrier
         * Loads/Stores ahead of the barrier must complete before the barrier.
         * Loads/Stores beyond the barrier must wait for the barrier to complete.
         */
        ALWAYS_INLINE
        static inline void fmb() { asm volatile ("dmb ish"   : : : "memory"); }

        /*
         * Read Memory Barrier
         * Loads ahead of the barrier must complete before the barrier.
         * Loads/Stores beyond the barrier must wait for the barrier to complete.
         */
        ALWAYS_INLINE
        static inline void rmb() { asm volatile ("dmb ishld" : : : "memory"); }

        /*
         * Write Memory Barrier
         * Stores ahead of the barrier must complete before the barrier.
         * Stores beyond the barrier must wait for the barrier to complete.
         */
        ALWAYS_INLINE
        static inline void wmb() { asm volatile ("dmb ishst" : : : "memory"); }

        /*
         * Full Memory Barrier + Synchronization
         * Like fmb(), but additionally instructions beyond the barrier must wait for the barrier to complete.
         */
        ALWAYS_INLINE
        static inline void fmb_sync() { asm volatile ("dsb sy" : : : "memory"); }

        /*
         * Read Memory Barrier + Synchronization
         * Like rmb(), but additionally instructions beyond the barrier must wait for the barrier to complete.
         */
        ALWAYS_INLINE
        static inline void rmb_sync() { asm volatile ("dsb ld" : : : "memory"); }

        /*
         * Write Memory Barrier + Synchronization
         * Like wmb(), but additionally instructions beyond the barrier must wait for the barrier to complete.
         */
        ALWAYS_INLINE
        static inline void wmb_sync() { asm volatile ("dsb st" : : : "memory"); }
};
