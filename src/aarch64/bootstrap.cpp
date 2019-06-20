/*
 * Bootstrap Code
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

#include "acpi.hpp"
#include "compiler.hpp"
#include "ec.hpp"
#include "interrupt.hpp"
#include "smmu.hpp"

extern "C" [[noreturn]]
void bootstrap (unsigned i, unsigned e)
{
    Cpu::init (i, e);

    // Once initialized, each core can handle its assigned interrupts
    Interrupt::init();

    // Before cores leave the barrier into userland, the SMMU must be active
    if (Cpu::bsp)
        Smmu::initialize();

    // Barrier: wait for all CPUs to arrive here
    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    if (!Acpi::resume) {

        // Create idle EC
        Ec::create_idle();

        // Create root EC
        if (Cpu::bsp)
            Ec::create_root();
    }

    Scheduler::schedule();
}
