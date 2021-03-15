/*
 * Bootstrap Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "ec.hpp"
#include "hip.hpp"

extern "C" NORETURN
void bootstrap()
{
    static Atomic<unsigned> barrier { 0 };

    Cpu::init();

    Ec::create_idle();

    // Barrier: wait for all ECs to arrive here
    for (++barrier; barrier != Cpu::online; pause()) ;

    // Create root task
    if (Cpu::bsp) {
        Hip::hip->add_check();
        Ec::create_root();
    }

    Sc::schedule();
}
