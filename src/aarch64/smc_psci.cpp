/*
 * SMC PSCI Calls (Power State Coordination Interface)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "extern.hpp"
#include "kmem.hpp"
#include "smc_arch.hpp"
#include "smc_psci.hpp"
#include "stdio.hpp"

bool Smc_psci::boot_cpu (cpu_t cpu, uint64_t mpidr)
{
    auto const aff { Cpu::affinity_bits (mpidr) };

    switch (cpu_on (aff, Kmem::sym_to_phys (&__init_psci), cpu)) {

        case Status::ALREADY_ON:
            Cpu::boot_cpu = cpu;
            return true;

        case Status::SUCCESS:
            trace (TRACE_FIRM | TRACE_PARSE, "PSCI: Booting CPU%u (%#llx)", cpu, aff);
            return true;

        default:
            return false;
    }
}

void Smc_psci::offline_wait()
{
    for (cpu_t i { 0 }; i < Cpu::count; i++) {

        if (Cpu::id == i)
            continue;

        while (affinity_info (Cpu::affinity_bits (Cpu::remote_mpidr (i))) != Status::AFFINITY_INFO_OFF)
            pause();
    }
}

void Smc_psci::init()
{
    auto const v { version() };

    if (v >= 0x2) [[likely]]            // PSCI 0.2+
        states |= BIT_RANGE (5, 4) | BIT (0);

    if (v >= 0x10000) [[likely]] {      // PSCI 1.0+

        // Determine if SYSTEM_SUSPEND is implemented
        if (std::to_underlying (features (function (Function64::SYSTEM_SUSPEND))) >= 0) [[likely]]
            states |= BIT_RANGE (3, 2);

        // Determine if SMCCC_VERSION is implemented
        if (std::to_underlying (features (Smc_arch::function (Smc_arch::Function32::VERSION))) >= 0) [[likely]]
            Smc_arch::init();
    }

    trace (TRACE_FIRM, "PSCI: Version %u.%u States %#x", v >> 16, v & BIT_RANGE (15, 0), states);
}
