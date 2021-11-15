/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "lowlevel.hpp"
#include "macros.hpp"
#include "types.hpp"

class Acpi_fixed final
{
    private:
        static inline constinit Atomic<bool> wak_sts { false };

    public:
        class Transition final
        {
            private:
                uint16_t val { 0 };

            public:
                bool valid() const { return BIT (15)         & val; }
                auto state() const { return BIT_RANGE (2, 0) & val; }

                explicit constexpr Transition() = default;
                explicit constexpr Transition (uint16_t v) : val { v } {}
        };

        /*
         * Perform platform sleep
         */
        static bool sleep (Transition)
        {
            wak_sts = true;                 // FIXME: Unimplemented

            return false;
        }

        static void wake_clr()
        {
            wak_sts = false;                // FIXME: Unimplemented
        }

        static void wake_chk()
        {
            for (; !wak_sts; pause()) ;     // FIXME: Unimplemented
        }
};
