/*
 * Interrupt Handling
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
        class Config
        {
            private:
                uint64 val;

            public:
                inline auto cpu() const { return static_cast<uint16>(val >> 48 & BIT_RANGE (15, 0)); }
                inline auto rid() const { return static_cast<uint16>(val >> 32 & BIT_RANGE (15, 0)); }
                inline bool gst() const { return val & BIT (3); }
                inline bool pol() const { return val & BIT (2); }
                inline bool trg() const { return val & BIT (1); }
                inline bool msk() const { return val & BIT (0); }

                ALWAYS_INLINE inline Config() = default;
                ALWAYS_INLINE inline Config (uint16 c, uint16 r, uint8 f) : val (static_cast<uint64>(c) << 48 | static_cast<uint64>(r) << 32 | f) {}
        };

        enum Request
        {
            RRQ,
            RKE,
        };

        Sm *            sm      { nullptr };
        Atomic<Config>  config  { Config (0, 0, BIT (0)) };

        static Interrupt int_table[NUM_SPI];

        static unsigned num_pin();
        static unsigned num_msi() { return 0; }

        static void init();

        static Event::Selector handler (bool);

        static bool get_act_tmr();
        static void set_act_tmr (bool);

        static void conf_sgi (unsigned, bool);
        static void conf_ppi (unsigned, bool, bool);
        static void conf_spi (unsigned, bool, bool, unsigned);

        static void configure (unsigned, Config, uint32 &, uint16 &);

        static void deactivate (unsigned);

        static void send_cpu (Request, unsigned);
        static void send_exc (Request);
};
