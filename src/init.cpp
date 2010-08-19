/*
 * Initialization Code
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#include "acpi.h"
#include "compiler.h"
#include "gsi.h"
#include "hip.h"
#include "idt.h"
#include "keyb.h"
#include "multiboot.h"
#include "ptab.h"

char const *version = "NOVA 0.3 (Cleetwood Cove)";

extern "C" INIT
mword kern_ptab_setup()
{
    Ptab *ptab = new Ptab;

    // Allocate and map cpu page
    ptab->update (CPULC_ADDR, 0,
                  Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
                  Ptab::Attribute (Ptab::ATTR_NOEXEC   |
                                   Ptab::ATTR_GLOBAL   |
                                   Ptab::ATTR_WRITABLE |
                                   Ptab::ATTR_PRESENT));

    // Allocate and map kernel stack
    ptab->update (KSTCK_ADDR, 0,
                  Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
                  Ptab::Attribute (Ptab::ATTR_NOEXEC   |
                                   Ptab::ATTR_GLOBAL   |
                                   Ptab::ATTR_WRITABLE |
                                   Ptab::ATTR_PRESENT));

    // Sync kernel code and data
    ptab->sync_master_range (LINK_ADDR, LOCAL_SADDR);

    return Buddy::ptr_to_phys (ptab);
}

extern "C" INIT REGPARM (1)
void init (mword mbi)
{
    for (void (**func)() = &CTORS_E; func != &CTORS_G; (*--func)()) ;

    // Setup 0-page and 1-page
    memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE);
    memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE);

    extern char __start_ap, __start_rlp;
    memcpy (reinterpret_cast<void *>(0x1000), &__start_ap, &__start_rlp - &__start_ap);

    // Setup consoles
    serial.init();
    screen.init();

     // Now we're ready to talk to the world
    printf ("\f%s: %s %s [%s]\n\n", version, __DATE__, __TIME__, COMPILER_STRING);

    Hip::build (mbi);

    Idt::build();
    Gsi::setup();
    Acpi::setup();

    Keyb::init();
}
