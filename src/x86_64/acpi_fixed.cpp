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

#include "acpi_fixed.hpp"

unsigned Acpi_fixed::read (Register reg)
{
    switch (reg) {
        case Register::PM1_STS:
            return hw_read (&pm1a_sts) | hw_read (&pm1b_sts);
        case Register::PM1_ENA:
            return hw_read (&pm1a_ena) | hw_read (&pm1b_ena);
        case Register::PM1_CNT:
            return hw_read (&pm1a_cnt) | hw_read (&pm1b_cnt);
        case Register::PM2_CNT:
            return hw_read (&pm2_cnt);
        case Register::PM_TMR:
            return hw_read (&pm_tmr);
        case Register::RESET:
            break;
    }

    return 0;
}

void Acpi_fixed::write (Register reg, unsigned val)
{
    // FIXME: Spec requires that certain bits be preserved.

    switch (reg) {
        case Register::PM1_STS:
            hw_write (&pm1a_sts, val);
            hw_write (&pm1b_sts, val);
            break;
        case Register::PM1_ENA:
            hw_write (&pm1a_ena, val);
            hw_write (&pm1b_ena, val);
            break;
        case Register::PM1_CNT:
            hw_write (&pm1a_cnt, val);
            hw_write (&pm1b_cnt, val);
            break;
        case Register::PM2_CNT:
            hw_write (&pm2_cnt, val);
            break;
        case Register::PM_TMR:          // read-only
            break;
        case Register::RESET:
            hw_write (&reset_reg, val);
            break;
    }
}

unsigned Acpi_fixed::hw_read (Acpi_gas *gas)
{
    if (!gas->bits)     // Register not implemented
        return 0;

    if (gas->asid == Acpi_gas::Asid::IO) {
        switch (gas->bits) {
            case 8:
                return Io::in<uint8>(static_cast<unsigned>(gas->addr));
            case 16:
                return Io::in<uint16>(static_cast<unsigned>(gas->addr));
            case 32:
                return Io::in<uint32>(static_cast<unsigned>(gas->addr));
        }
    }

    return 0;
}

void Acpi_fixed::hw_write (Acpi_gas *gas, unsigned val)
{
    if (!gas->bits)     // Register not implemented
        return;

    if (gas->asid == Acpi_gas::Asid::IO) {
        switch (gas->bits) {
            case 8:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint8>(val));
                return;
            case 16:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint16>(val));
                return;
            case 32:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint32>(val));
                return;
        }
    }
}
