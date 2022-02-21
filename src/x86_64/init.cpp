/*
 * Initialization Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "cmdline.hpp"
#include "compiler.hpp"
#include "extern.hpp"
#include "ioapic.hpp"
#include "interrupt.hpp"
#include "patch.hpp"
#include "pic.hpp"
#include "smmu.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"
#include "string.hpp"

extern "C" uintptr_t kern_ptab_setup (apic_t t)
{
    if (Acpi::resume)
        return Space_hst::nova.loc[Cpu::find_by_topology (t)].root_addr();

    Hptp hptp;

    // Share kernel code and data
    hptp.share_from_master (LINK_ADDR, MMAP_CPU);

    // Allocate and map cpu page
    hptp.update (MMAP_CPU_DATA, Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram());

#if defined(__CET__) && (__CET__ & 2)
    // Allocate and map kernel shadow stack
    auto const sstk { static_cast<uintptr_t *>(Buddy::alloc (0, Buddy::Fill::BITS0)) };
    hptp.update (MMAP_CPU_SSTK, Kmem::ptr_to_phys (sstk), 0,
                 Paging::Permissions (Paging::SS | Paging::G | Paging::R), Memattr::ram());

    // Install supervisor shadow stack token
    sstk[PAGE_SIZE (0) / sizeof (uintptr_t) - 1] = reinterpret_cast<uintptr_t>(&SSTK_TOP);
#endif

    // Allocate and map kernel data stack
    hptp.update (MMAP_CPU_DSTK, Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::ram());

    return hptp.root_addr();
}

extern "C" void preinit()
{
    Patch::detect();
}

extern "C" void init()
{
    if (!Acpi::resume) {

        Patch::init();
        Buddy::init();

        for (void (**func)() = &CTORS_S; func != &CTORS_E; (*func++)()) ;

        Cmdline::init();

        for (void (**func)() = &CTORS_C; func != &CTORS_S; (*func++)()) ;

        // Now we're ready to talk to the world
        Console::print ("\nNOVA Microhypervisor #%07lx (%s): %s %s [%s]\n", reinterpret_cast<uintptr_t>(&GIT_VER), ARCH, __DATE__, __TIME__, COMPILER_STRING);

        Interrupt::setup();
    }

    Acpi::init();

    Pic::init();

    Ioapic::init_all();

    Smmu::init_all();

    Interrupt::init_all();
}
