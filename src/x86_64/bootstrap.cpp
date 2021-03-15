/*
 * Bootstrap Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "hip.hpp"
#include "timer.hpp"

extern "C" [[noreturn]]
void bootstrap()
{
    Cpu::init();

    // Before cores leave the barrier into userland, the idle EC must exist
    if (!Acpi::resume)
        Ec::create_idle();

    // Barrier: wait for all CPUs to arrive here
    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    if (Acpi::resume)
        Timer::set_time (Acpi::resume);

    else if (Cpu::bsp) {
        Hip::hip->add_check();
        Ec::create_root();
    }

    if (Cpu::bsp)
        Acpi::wake_restore();

    Scheduler::schedule();
}
