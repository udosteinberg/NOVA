/*
 * Bootstrap Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "ec.hpp"

extern "C" [[noreturn]] void bootstrap()
{
    Cpu::init();

    // Idle EC must exist before scheduler invocation
    if (Acpi::resume)
        Space_hst::current = nullptr;
    else {
        Ec::current = new Ec (Pd::current = &Pd::kern, Ec::idle, Cpu::id);
        Sc::current = new Sc (&Pd::kern, Cpu::id, Ec::current);
    }

    if (Cpu::bsp) [[unlikely]] {

        // Barrier: wait for all non-BSP CPUs to arrive here
        for (; Cpu::online != Cpu::count - 1; pause()) ;

        // Waking vector must be restored before CPUs pass barrier into userland
        Acpi::wake_restore();

        // SMMU must be active before CPUs pass barrier into userland
        if (!Smmu::initialize()) [[unlikely]]
            panic ("SMMU initialization failed");
    }

    // Barrier: wait for all CPUs to arrive here
    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    if (Acpi::resume)
        Timer::set_time (Acpi::resume);

    else if (Cpu::bsp) {
        Hip::hip->add_check();
        Ec *root_ec = new Ec (&Pd::root, NUM_EXC + 1, &Pd::root, Ec::root_invoke, Cpu::id, 0, USER_ADDR - 2 * PAGE_SIZE (0), 0);
        Sc *root_sc = new Sc (&Pd::root, NUM_EXC + 2, root_ec, Cpu::id, Sc::default_prio, Sc::default_quantum);
        root_sc->remote_enqueue();
    }

    Sc::schedule();
}
