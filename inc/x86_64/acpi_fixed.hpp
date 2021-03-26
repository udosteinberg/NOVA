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

#include "acpi_gas.hpp"
#include "io.hpp"
#include "lowlevel.hpp"
#include "macros.hpp"

class Acpi_fixed
{
    friend class Acpi_table_fadt;

    private:
        enum class Register
        {
            PM1_STS,
            PM1_ENA,
            PM1_CNT,
            PM2_CNT,
            PM_TMR,
            RESET,
        };

        static inline Acpi_gas pm1a_sts, pm1b_sts;
        static inline Acpi_gas pm1a_ena, pm1b_ena;
        static inline Acpi_gas pm1a_cnt, pm1b_cnt;
        static inline Acpi_gas pm2_cnt;
        static inline Acpi_gas pm_tmr;
        static inline Acpi_gas reset_reg;
        static inline uint8    reset_val;

        static constexpr unsigned timer_frequency = 3579545;

        static unsigned hw_read (Acpi_gas *);
        static unsigned read (Register);

        static void hw_write (Acpi_gas *, unsigned);
        static void write (Register, unsigned);

    public:
        static inline void delay (unsigned ms)
        {
            unsigned cnt = timer_frequency * ms / 1000;
            unsigned val = read (Register::PM_TMR);

            while ((read (Register::PM_TMR) - val) % BIT (24) < cnt)
                pause();
        }

        static inline void reset()
        {
            write (Register::RESET, reset_val);
        }

        static inline bool enabled()
        {
            return read (Register::PM1_CNT) & BIT (0);
        }

        static void enable (unsigned port, uint8 cmd)
        {
            if (!port || !cmd || enabled())
                return;

            Io::out (port, cmd);

            while (!enabled())
                pause();
        }
};
