/*
 * Central Processing Unit (CPU)
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

#include "arch.hpp"
#include "compiler.hpp"
#include "types.hpp"

class Cpu
{
    private:
        static uint64   midr                CPULOCAL;           // Main ID Register
        static uint64   mpidr               CPULOCAL;           // Multiprocessor Affinity Register

    public:
        static inline unsigned          boot_cpu    { 0 };
        static inline unsigned          count       { 0 };

        // Returns affinity in Aff3[31:24] Aff2[23:16] Aff1[15:8] Aff0[7:0] format
        ALWAYS_INLINE
        static inline auto affinity() { return static_cast<uint32>((mpidr >> 8 & BIT_RANGE (31, 24)) | (mpidr & BIT_RANGE (23, 0))); }

        // Returns affinity in Aff3[39:32] Aff2[23:16] Aff1[15:8] Aff0[7:0] format
        ALWAYS_INLINE
        static inline auto affinity_bits (uint64 v) { return v & (BIT64_RANGE (39, 32) | BIT64_RANGE (23, 0)); }

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
