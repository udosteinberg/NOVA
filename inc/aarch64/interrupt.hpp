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
#include "smmu.hpp"
#include "status.hpp"

class Dc;
class Sm;

class Interrupt final
{
    private:
        static inline constinit Bitmap<Intid::NUM_SPI,  false> guest_s;
        static inline constinit Bitmap<Intid::NUM_ESPI, false> guest_e;

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

        static void *get_ptr (Intid i)
        {
            // No interrupt semaphores for SMMU IIDs
            if (Smmu::using_iid (i)) [[unlikely]]
                return nullptr;

            switch (Intid::type (i)) {

                default:
                    return nullptr;
            }
        }

        static bool valid (Intid i) { return get_ptr (i) != nullptr; }

        static bool tmr_act_get();
        static void tmr_act_set (bool);

        static Event::Selector handler (bool);

        static Status assign (bool, Sm *, Dc const *, uint16_t, uint16_t, uint8_t, uint8_t, uintptr_t &, uintptr_t &);
        static void deactivate (Sm *);

        static void send_cpu (Request, cpu_t);
        static void send_exc (Request);
};
