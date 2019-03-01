/*
 * Central Processing Unit (CPU)
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

#include "compiler.hpp"
#include "types.hpp"

class Cpu
{
    private:
        static uint64   midr                CPULOCAL;           // Main ID Register
        static uint64   mpidr               CPULOCAL;           // Multiprocessor Affinity Register

    public:
        ALWAYS_INLINE
        static inline void preemption_disable() { asm volatile ("msr daifset, #0xf" : : : "memory"); }

        ALWAYS_INLINE
        static inline void preemption_enable() { asm volatile ("msr daifclr, #0xf" : : : "memory"); }

        ALWAYS_INLINE
        static inline void preemption_point() { asm volatile ("msr daifclr, #0xf; msr daifset, #0xf" : : : "memory"); }

        ALWAYS_INLINE
        static inline void halt() { asm volatile ("wfi; msr daifclr, #0xf; msr daifset, #0xf" : : : "memory"); }

        static void init();
        static void fini();
};
