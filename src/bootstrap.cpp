/*
 * Bootstrap Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "ec.h"
#include "hip.h"
#include "msr.h"

extern "C" NORETURN
void bootstrap()
{
    static mword barrier;

    Cpu::init();

    // Create idle EC
    Ec::current = new Ec (&Pd::kern, 0, Cpu::id, 0, Ec::idle);
    Sc::current = new Sc (&Pd::kern, 0, Ec::current, Cpu::id, 0, 1000000);

    // Barrier: wait for all ECs to arrive here
    for (Atomic::add (barrier, 1UL); barrier != Cpu::online; pause()) ;

    Msr::write<uint64>(Msr::IA32_TSC, 0);

    // Create root task
    if (Cpu::bsp) {
        Hip::add_check();
        Ec *root_ec = new Ec (&Pd::root, NUM_EXC + 1, Cpu::id, USER_ADDR - 2 * PAGE_SIZE, 0, 0);
        Sc *root_sc = new Sc (&Pd::root, NUM_EXC + 2, root_ec, Cpu::id, Sc::default_prio, Sc::default_quantum);
        root_ec->set_cont (Ec::root_invoke);
        root_sc->remote_enqueue();
    }

    Sc::schedule();
}
