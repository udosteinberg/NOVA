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

#include "cpu.hpp"
#include "gits.hpp"

extern "C" [[noreturn]] void bootstrap (cpu_t c)
{
    Cpu::init (c);

    if (Cpu::bsp) [[unlikely]] {

        // Barrier: wait up to 5s for all non-BSP CPUs to arrive here
        if (!Wait::until (5000, [] { return Cpu::online == Cpu::count - 1; })) [[unlikely]]
            panic ("Failed to start all APs");

        // GITS must be active before CPUs pass barrier into userland
        if (!Gits::initialize()) [[unlikely]]
            panic ("GITS initialization failed");
    }

    // Barrier: wait for all CPUs to arrive here
    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    for (;;) ;
}
