/*
 * Global System Interrupts (GSI)
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

#pragma once

#include "assert.h"
#include "compiler.h"

class Ioapic;
class Sm;

class Gsi
{
    private:
        static unsigned row;

    public:
        Ioapic *        ioapic;
        Sm *            sm;
        uint8           vec;
        uint8           msk : 1,
                        trg : 1,
                        pol : 1;

        static Gsi      gsi_table[NUM_GSI];
        static unsigned irq_table[NUM_IRQ];

        INIT
        static void setup();

        INIT
        static void init();

        static uint64 set (unsigned long, unsigned long = 0, unsigned = 0);

        static void mask (unsigned long);
        static void unmask (unsigned long);

        ALWAYS_INLINE
        static inline unsigned irq_to_gsi (unsigned irq)
        {
            assert (irq < NUM_IRQ);
            return irq_table[irq];
        }

        REGPARM (1)
        static void vector (unsigned) asm ("gsi_vector");
};
