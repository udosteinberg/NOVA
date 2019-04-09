/*
 * Interrupt Handling
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

#pragma once

#include "atomic.hpp"
#include "event.hpp"
#include "intid.hpp"
#include "macros.hpp"
#include "types.hpp"

class Sm;

class Interrupt : private Intid
{
    private:
        static void rke_handler();

        static Event::Selector handle_sgi (uint32, bool);
        static Event::Selector handle_ppi (uint32, bool);
        static Event::Selector handle_spi (uint32, bool);

    public:
        enum Request
        {
            RRQ,
            RKE,
        };

        Sm *            sm      { nullptr };
        Atomic<uint32>  config  { 0xffff };

        inline auto cpu() const { return config & BIT_RANGE (15, 0); }
        inline bool msk() const { return config & BIT (16); }
        inline bool trg() const { return config & BIT (17); }
        inline bool pol() const { return config & BIT (18); }
        inline bool gst() const { return config & BIT (19); }

        static Interrupt int_table[NUM_SPI];

        static unsigned count();

        static Event::Selector handler (bool);

        static void conf_sgi (unsigned, bool);
        static void conf_ppi (unsigned, bool, bool);
        static void conf_spi (unsigned, unsigned, bool, bool);

        static void configure (unsigned, unsigned, uint16, uint16, uint32 &, uint16 &);

        static void deactivate (unsigned);

        static void send_cpu (Request, unsigned);
        static void send_exc (Request);
};
