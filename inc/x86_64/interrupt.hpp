/*
 * Interrupt Handling
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

#pragma once

#include "config.hpp"
#include "types.hpp"

class Ioapic;
class Sm;

class Interrupt
{
    private:
        static void rke_handler();

        static void set_mask (unsigned, bool);

    public:
        enum Request
        {
            RRQ,
            RKE,
        };

        Sm *        sm      { nullptr };
        Ioapic *    ioapic  { nullptr };
        uint16      cpu     { 0xffff };
        uint16      rte     { 0 };

        static Interrupt int_table[NUM_GSI];

        static unsigned count() { return NUM_GSI; }

        static void init();

        static void handle_ipi (unsigned) asm ("ipi_vector");
        static void handle_lvt (unsigned) asm ("lvt_vector");
        static void handle_gsi (unsigned) asm ("gsi_vector");

        static void configure (unsigned, unsigned, unsigned, uint16, uint32 &, uint16 &);

        static inline void deactivate (unsigned gsi) { set_mask (gsi, false); }

        static void send_cpu (unsigned, Request);
};
