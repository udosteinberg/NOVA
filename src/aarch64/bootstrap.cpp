/*
 * Bootstrap Code
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "atomic.hpp"
#include "compiler.hpp"
#include "cpu.hpp"
#include "ec.hpp"
#include "lowlevel.hpp"
#include "pd.hpp"
#include "sc.hpp"

extern "C" NORETURN
void bootstrap (unsigned i, unsigned e)
{
    static mword barrier;

    Cpu::init (i, e);

    Ec::current = Ec::create (Cpu::id, Ec::idle);
    Sc::current = Sc::create (Cpu::id, Ec::current, 0, 1000);

    // Barrier: wait for all CPUs to arrive here
    for (Atomic::add (barrier, 1UL); barrier != Cpu::online; pause()) ;

    if (Cpu::bsp) {
        Ec *root_ec = Ec::create (&Pd::root, true, Cpu::id, 0, (Space_mem::num - 2) << PAGE_BITS, 0, Ec::root_invoke);
        Sc *root_sc = Sc::create (Cpu::id, root_ec, Sc::max_prio(), 1000);
        root_sc->remote_enqueue();
    }

    Sc::schedule();
}
