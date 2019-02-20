/*
 * Central Processing Unit (CPU)
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

#include "arch.hpp"
#include "atomic.hpp"
#include "kmem.hpp"
#include "types.hpp"

class Cpu final
{
    private:
        static uint64_t ptab                CPULOCAL;           // EL2 Page Table Root
        static uint64_t midr                CPULOCAL;           // Main ID Register
        static uint64_t mpidr               CPULOCAL;           // Multiprocessor Affinity Register

    public:
        static inline cpu_t             boot_cpu    { 0 };
        static inline cpu_t             count       { 0 };
        static inline Atomic<cpu_t>     online      { 0 };

        // Returns affinity in Aff3[31:24] Aff2[23:16] Aff1[15:8] Aff0[7:0] format
        static constexpr auto affinity_pack (uint64_t v) { return static_cast<uint32_t>((v >> 8 & BIT_RANGE (31, 24)) | (v & BIT_RANGE (23, 0))); }

        // Returns affinity in Aff3[39:32] Aff2[23:16] Aff1[15:8] Aff0[7:0] format
        static constexpr auto affinity_bits (uint64_t v) { return v & (BIT64_RANGE (39, 32) | BIT64_RANGE (23, 0)); }

        static auto remote_mpidr (cpu_t cpu) { return *Kmem::loc_to_glob (cpu, &mpidr); }

        static auto remote_ptab (cpu_t cpu) { return *Kmem::loc_to_glob (cpu, &ptab); }

        static void preemption_disable() { asm volatile ("msr daifset, #0xf" : : : "memory"); }

        static void preemption_enable() { asm volatile ("msr daifclr, #0xf" : : : "memory"); }

        static void preemption_point() { asm volatile ("msr daifclr, #0xf; msr daifset, #0xf" : : : "memory"); }

        static void halt() { asm volatile ("wfi; msr daifclr, #0xf; msr daifset, #0xf" : : : "memory"); }

        static void init();
        static void fini();
};
