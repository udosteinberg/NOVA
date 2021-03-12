/*
 * Bootstrap Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "space_obj.hpp"
#include "timer.hpp"

extern "C" [[noreturn]]
void bootstrap()
{
    Cpu::init();

    // Before cores leave the barrier into userland, the idle EC must exist
    if (!Acpi::resume) {
        Ec::current = new Ec (&Space_hst::nova, Ec::idle, Cpu::id);
        Sc::current = new Sc (nullptr, Cpu::id, Ec::current);
    }

    // Barrier: wait for all CPUs to arrive here
    for (Cpu::online++; Cpu::online != Cpu::count; pause()) ;

    if (Acpi::resume)
        Timer::set_time (Acpi::resume);

    else if (Cpu::bsp) {
        Hip::hip->add_check();
        Status s;
        Pd::root = Pd::create (s);
        Space_obj::nova.insert (Space_obj::Selector::ROOT_PD, Capability (Pd::root, std::to_underlying (Capability::Perm_pd::DEFINED)));
        Pd::root->create_obj (s, &Space_obj::nova, Space_obj::Selector::ROOT_OBJ);
        Pd::root->create_hst (s, &Space_obj::nova, Space_obj::Selector::ROOT_HST);
        Pd::root->create_pio (s, &Space_obj::nova, Space_obj::Selector::ROOT_PIO);
        Ec *root_ec = new Ec (Pd::root->get_obj(), Pd::root->get_hst(), Pd::root->get_pio(), NUM_EXC + 1, Ec::root_invoke, Cpu::id, 0, USER_ADDR - 2 * PAGE_SIZE (0), 0);
        Sc *root_sc = new Sc (Pd::root, NUM_EXC + 2, root_ec, Cpu::id, Sc::default_prio, Sc::default_quantum);
        root_sc->remote_enqueue();
    }

    if (Cpu::bsp)
        Acpi::wake_restore();

    Sc::schedule();
}
