/*
 * Interrupt Handling
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "macros.hpp"
#include "types.hpp"
#include "vectors.hpp"

class Ioapic;
class Sm;

class Interrupt
{
    private:
        static void rke_handler();

        static void handle_ipi (unsigned);
        static void handle_lvt (unsigned);
        static void handle_gsi (unsigned);

        static void set_mask (unsigned, bool);

        static void init (unsigned, uint32 &, uint16 &);

    public:
        class Config
        {
            private:
                uint64 val;

            public:
                inline auto cpu() const { return static_cast<uint16>(val >> 48 & BIT_RANGE (15, 0)); }
                inline auto rid() const { return static_cast<uint16>(val >> 32 & BIT_RANGE (15, 0)); }
                inline bool gst() const { return false; }
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
            uint32 addr; uint16 data;
            for (unsigned i = 0; i < NUM_GSI; i++)
                init (i, addr, data);
        }

        static void handler (unsigned) asm ("int_handler");

        static void configure (unsigned, Config, uint32 &, uint16 &);

        static inline void deactivate (unsigned gsi) { set_mask (gsi, false); }

        static void send_cpu (Request, unsigned);
        static void send_exc (Request);
};
