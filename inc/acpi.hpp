/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_arch.hpp"
#include "acpi_fixed.hpp"
#include "atomic.hpp"

class Acpi final : public Acpi_arch
{
    friend class Acpi_table_fadt;

    private:
        static inline uint32 fflg { 0 };    // Feature Flags
        static inline uint16 bflg { 0 };    // Boot Flags

        static inline Atomic<uint16> state { 0 };

    public:
        static inline uint16 get_state() { return state; }

        static inline void clr_state()
        {
            Acpi_fixed::wake_clr();

            state = 0;
        }

        static inline bool set_state (uint16 s)
        {
            uint16 o = 0;

            return state.compare_exchange (o, s);
        }

        static bool init();

        static void sleep();
};
