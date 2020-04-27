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

    public:
        static inline uint64 resume { 0 };

        class Transition
        {
            private:
                uint16 val;

            public:
                inline auto state() const { return val      & BIT_RANGE (2, 0); }
                inline auto val_a() const { return val >> 3 & BIT_RANGE (2, 0); }
                inline auto val_b() const { return val >> 6 & BIT_RANGE (2, 0); }

                ALWAYS_INLINE inline explicit Transition() = default;
                ALWAYS_INLINE inline explicit Transition (uint16 v) : val (v) {}
        };

        static inline Transition get_transition() { return trans; }

        static inline bool set_transition (Transition s)
        {
            Transition o (0);

            return trans.compare_exchange (o, s);
        }

        static inline void clr_transition()
        {
            Acpi_fixed::wake_clr();

            trans = Transition (0);
        }

        static bool init();
        static void fini (Transition);

    private:
        static inline uint32 fflg { 0 };    // Feature Flags
        static inline uint16 bflg { 0 };    // Boot Flags

        static inline Atomic<Transition> trans { Transition (0) };
};
