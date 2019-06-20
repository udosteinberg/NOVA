/*
 * Bootstrap Code
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

#include "acpi.hpp"
#include "compiler.hpp"
#include "cpu.hpp"
#include "ec.hpp"
#include "gits.hpp"
#include "smmu.hpp"

extern "C" [[noreturn]] void bootstrap (cpu_t c)
{
    Cpu::init (c);

    // Idle EC must exist before scheduler invocation
    if (!Acpi::resume) [[likely]]
        Ec::create_idle();

    if (Cpu::bsp) [[unlikely]] {

        // Barrier: wait for all non-BSP CPUs to arrive here
        for (; Cpu::online != Cpu::count - 1; pause()) ;

        // GITS must be active before CPUs pass barrier into userland
        if (!Gits::initialize()) [[unlikely]]
            panic ("GITS initialization failed");

        // SMMU must be active before CPUs pass barrier into userland
        if (!Smmu::initialize()) [[unlikely]]
            panic ("SMMU initialization failed");
    }

    // Barrier: wait for all CPUs to arrive here
    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    if (!Acpi::resume)
        if (Cpu::bsp)
            Ec::create_root();

    Scheduler::schedule();
}
