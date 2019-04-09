/*
 * Interrupt Handling
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
#include "status.hpp"
#include "types.hpp"

class Sm;

class Interrupt final : private Intid
{
    private:
        static inline constinit Bitmap<NUM_SPI> guest_owned;

        static void rke_handler();

        static Event::Selector handle_sgi (uint32_t, bool);
        static Event::Selector handle_ppi (uint32_t, bool);
        static Event::Selector handle_spi (uint32_t, bool);

    public:
        static        constexpr gsi_t vec_max { 0 };
        static inline constinit gsi_t gsi_max { 0 };
        static inline constinit gsi_t gsi_pin { 0 };

        enum Request
        {
            RRQ,
            RKE,
        };

        static void conf_sgi (unsigned, bool);
        static void conf_ppi (unsigned, bool, bool);
        static void conf_spi (unsigned, bool, bool, cpu_t);

        static Event::Selector handler (bool);

        static Status assign (Sm *, cpu_t, gsi_t, pci_t, uint8_t, uintptr_t &, uintptr_t &);
        static void deactivate (gsi_t);

        static void send_cpu (Request, cpu_t);
        static void send_exc (Request);
};
