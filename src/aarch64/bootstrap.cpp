/*
 * Bootstrap Code
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

extern "C" NORETURN
void bootstrap (unsigned i, unsigned e)
{
    static Atomic<unsigned> barrier { 0 };

    Cpu::init (i, e);

    Ec::create_idle();

    // Barrier: wait for all CPUs to arrive here
    for (++barrier; barrier != Cpu::online; pause()) ;

    if (Cpu::bsp)
        Ec::create_root();

    Sc::schedule();
}
