/*
 * Bootstrap Code
 *
 * Copyright (C) 2005-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "compiler.h"
#include "cpu.h"
#include "ec.h"
#include "hip.h"
#include "msr.h"
#include "pd.h"
#include "sc.h"
#include "x86.h"

extern "C" NORETURN
void bootstrap()
{
    Cpu::init();

    // Create idle EC
    Ec::current = new Ec (&Pd::kern, Cpu::id, 0, Ec::idle);
    Sc::current = new Sc (Ec::current, Cpu::id, 0, 1000000);

    // Barrier: wait for all ECs to arrive here
    for (Atomic::sub (Cpu::boot_count, 1); Cpu::boot_count; pause()) ;

    Msr::write<uint64>(Msr::IA32_TSC, 0);

    // Create root task
    if (Cpu::bsp) {
        Hip::add_check();
        Pd::root = new Pd (true);
        Ec *root_ec = new Ec (Pd::root, Cpu::id, LINK_ADDR - 2 * PAGE_SIZE, 0, 0, false);
        Sc *root_sc = new Sc (root_ec, Cpu::id, Sc::default_prio, Sc::default_quantum);
        root_ec->set_continuation (Ec::root_invoke);
        root_ec->set_sc (root_sc);
        root_sc->ready_enqueue();
    }

    Sc::schedule();
}
