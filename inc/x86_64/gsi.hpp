/*
 * Global System Interrupts (GSI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "assert.hpp"
#include "vectors.hpp"

class Ioapic;
class Sm;

class Gsi
{
    private:
        static void handle_ipi (unsigned);
        static void handle_lvt (unsigned);
        static void handle_gsi (unsigned);

        static auto gsi_to_vec (unsigned gsi) { return static_cast<uint8_t>(gsi + VEC_GSI); }

    public:
        Sm *            sm;
        Ioapic *        ioapic;
        uint8_t         dst;
        bool            trg;
        bool            pol;

        static Gsi      gsi_table[NUM_GSI];
        static unsigned irq_table[NUM_IRQ];

        static void setup();

        static uint64 set (unsigned, cpu_t = 0);

        static void mask (unsigned);
        static void unmask (unsigned);

        ALWAYS_INLINE
        static inline unsigned irq_to_gsi (unsigned irq)
        {
            assert (irq < NUM_IRQ);
            return irq_table[irq];
        }

        static void handler (unsigned) asm ("int_handler");
};
