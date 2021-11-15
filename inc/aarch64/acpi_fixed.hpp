/*
 * Advanced Configuration and Power Interface (ACPI)
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
#include "lowlevel.hpp"
#include "macros.hpp"
#include "types.hpp"

class Acpi_fixed
{
    private:
        static inline Atomic<bool> wak_sts;

    public:
        class Transition
        {
            private:
                uint16_t val;

            public:
                auto state() const { return val & BIT_RANGE (2, 0); }

                explicit Transition() = default;
                explicit Transition (uint16_t v) : val (v) {}
        };

        static inline bool supported (Transition)
        {
            return false;
        }

        /*
         * Perform core powerdown
         */
        static inline bool pwrdn (Transition)
        {
            return false;
        }

        /*
         * Perform platform reset
         */
        static inline bool reset()
        {
            return false;
        }

        /*
         * Perform platform sleep
         */
        static inline bool sleep (Transition)
        {
            wak_sts = true;                 // FIXME: Unimplemented

            return false;
        }

        static inline void wake_clr()
        {
            wak_sts = false;                // FIXME: Unimplemented
        }

        static inline void wake_chk()
        {
            for (; !wak_sts; pause()) ;     // FIXME: Unimplemented
        }
};
