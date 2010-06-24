/*
 * IA32 Boot Page Table
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

#include "extern.h"
#include "ptab_boot.h"

void Ptab_boot::init()
{
    extern char _master_p;
    Ptab_boot *ptab = reinterpret_cast<Ptab_boot *>(&_master_p);

    // Map kernel to load address
    ptab->map (LOAD_ADDR,
               LOAD_ADDR,
               reinterpret_cast<mword>(&LOAD_SIZE),
               Ptab::ATTR_WRITABLE);

    // Map kernel to link address
    ptab->map (reinterpret_cast<mword>(&LINK_PHYS),
               LINK_ADDR,
               reinterpret_cast<mword>(&LINK_SIZE),
               Ptab::Attribute (Ptab::ATTR_GLOBAL |
                                Ptab::ATTR_WRITABLE));

    // Copy SMP startup code to trampoline page
    // XXX: Make sure frame 0x1000 is reserved for kernel use
    extern char __start_ap, __start_rlp;
    memcpy (reinterpret_cast<void *>(0x1000), &__start_ap, &__start_rlp - &__start_ap);
}

void Ptab_boot::map (Paddr phys, mword virt, size_t size, Attribute attrib)
{
    while (size) {

        unsigned lev = levels;

        for (Ptab_boot *pte = this;; pte = reinterpret_cast<Ptab_boot *>(pte->addr())) {

            unsigned shift = --lev * bpl + 12;
            pte += virt >> shift & ((1ul << bpl) - 1);
            size_t mask = (1ul << shift) - 1;

            if (size > mask && !((phys | virt) & mask)) {

                Paddr a = attrib | ATTR_LEAF;

                if (lev)
                    a |= ATTR_SUPERPAGE;

                pte->val = phys | a;

                mask++;
                size -= mask;
                virt += mask;
                phys += mask;
                break;
            }

            if (!pte->present())
                pte->val = reinterpret_cast<mword>(new Ptab_boot) | ATTR_PTAB;
        }
    }
}
