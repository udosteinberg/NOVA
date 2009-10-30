/*
 * Global System Interrupts (GSI)
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "apic.h"
#include "assert.h"
#include "compiler.h"
#include "types.h"
#include "vectors.h"

class Ioapic;
class Sm;

class Gsi : public Apic
{
    private:
        static unsigned row;

        enum
        {
            ELCR_MST = 0x4d0,
            ELCR_SLV = 0x4d1
        };

        ALWAYS_INLINE
        static inline void eoi (unsigned gsi);

    public:
        Sm *            sm;
        Ioapic *        ioapic;
        unsigned        irt;
        unsigned short  pin;
        unsigned short  cpu;

        static Gsi      gsi_table[NUM_GSI];
        static unsigned irq_table[NUM_IRQ];

        INIT
        static void setup();

        INIT
        static void init();

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
