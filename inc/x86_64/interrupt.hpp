/*
 * Interrupt Handling
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

#pragma once

#include "atomic.hpp"
#include "macros.hpp"
#include "types.hpp"
#include "vectors.hpp"

class Ioapic;
class Sm;

class Interrupt final
{
    private:
        static void rke_handler();

        static void handle_ipi (unsigned);
        static void handle_lvt (unsigned);
        static void handle_gsi (unsigned);

        static void set_mask (unsigned, bool);

        static void init (unsigned, uint32_t &, uint16_t &);

    public:
        class Config final
        {
            private:
                uint64_t val;

            public:
                inline auto cpu() const { return static_cast<uint16_t>(val >> 48); }
                inline auto src() const { return static_cast<uint16_t>(val >> 32); }
                inline bool gst() const { return false; }
                inline bool pol() const { return val & BIT (2); }
                inline bool trg() const { return val & BIT (1); }
                inline bool msk() const { return val & BIT (0); }

                Config() = default;
                Config (cpu_t c, uint16_t s, uint8_t f) : val (static_cast<uint64_t>(c) << 48 | static_cast<uint64_t>(s) << 32 | f) {}
        };

        enum Request
        {
            RRQ,
            RKE,
        };

        Sm *            sm      { nullptr };
        Ioapic *        ioapic  { nullptr };
        Atomic<Config>  config  { Config (0, 0, BIT (0)) };

        static inline unsigned pin { 0 };

        static Interrupt int_table[NUM_GSI];

        static inline unsigned num_pin() { return pin; }
        static inline unsigned num_msi() { return NUM_GSI - pin; }

        static void setup();

        ALWAYS_INLINE
        static inline void init_all()
        {
            uint32_t addr; uint16_t data;
            for (unsigned i { 0 }; i < NUM_GSI; i++)
                init (i, addr, data);
        }

        static void handler (unsigned) asm ("int_handler");

        static void configure (unsigned, Config, uint32_t &, uint16_t &);

        static inline void deactivate (unsigned gsi) { set_mask (gsi, false); }

        static void send_cpu (Request, cpu_t);
        static void send_exc (Request);
};
