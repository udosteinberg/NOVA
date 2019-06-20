/*
 * Interrupt Handling
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "bitmap.hpp"
#include "event.hpp"
#include "intid.hpp"
#include "sm.hpp"
#include "smmu.hpp"

class Dc;

class Interrupt final : private Intid
{
    private:
        static inline constinit Refptr<Sm> table_s[NUM_SPI];
        static inline constinit Refptr<Sm> table_e[NUM_ESPI];
        static inline constinit Refptr<Sm> table_l[NUM_LPI];

        static inline constinit Bitmap<NUM_SPI>  guest_s;
        static inline constinit Bitmap<NUM_ESPI> guest_e;

        static void rke_handler();

        static Event::Selector handle_sgi  (unsigned, auto const &);
        static Event::Selector handle_ppi  (unsigned, auto const &, bool);
        static Event::Selector handle_spi  (unsigned, auto const &);
        static Event::Selector handle_eppi (unsigned, auto const &);
        static Event::Selector handle_espi (unsigned, auto const &);
        static Event::Selector handle_lpi  (unsigned);

    public:
        static inline constinit unsigned num_spi    { 0 };
        static inline constinit unsigned num_eppi   { 0 };
        static inline constinit unsigned num_espi   { 0 };
        static inline constinit unsigned num_lpi    { 0 };

        enum Request
        {
            RRQ,
            RKE,
        };

        static Refptr<Sm> *get_ptr (arm_intid_t iid)
        {
            // No interrupt semaphores for SMMU IIDs
            if (Smmu::using_iid (iid)) [[unlikely]]
                return nullptr;

            unsigned n;

            switch (Intid::type (iid)) {

                case Intid::Type::SPI:
                    n = Intid::to_spi (iid);
                    return n < num_spi ? table_s + n : nullptr;

                case Intid::Type::ESPI:
                    n = Intid::to_espi (iid);
                    return n < num_espi ? table_e + n : nullptr;

                case Intid::Type::LPI:
                    n = Intid::to_lpi (iid);
                    return n < num_lpi ? table_l + n : nullptr;

                default:
                    return nullptr;
            }
        }

        static bool tmr_act_get();
        static void tmr_act_set (bool);

        static Event::Selector handler (bool);

        static Status assign (bool, Sm *, Dc const *, cpu_t, uint16_t, uint8_t, uintptr_t &, uintptr_t &);
        static void deactivate (Sm *);

        static void send_cpu (Request, cpu_t);
        static void send_exc (Request);
};
