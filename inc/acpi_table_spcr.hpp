/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_table.hpp"

/*
 * Serial Port Console Redirection Table (SPCR)
 */
class Acpi_table_spcr final
{
    private:
        Acpi_table                  table;                                  //  0
        Unaligned_le<uint16_t>      subtype;                                //  36
        Unaligned_le<uint16_t>      reserved;                               //  38
        Acpi_gas                    regs;                                   //  40
        Unaligned_le<uint8_t>       intr_type, irq, gsi[4];                 //  52
        Unaligned_le<uint8_t>       baud, parity, stop, flow, term, lang;   //  58
        Unaligned_le<uint16_t>      did, vid;                               //  64
        Unaligned_le<uint8_t>       bus, dev, fun, flags[4], seg, res[4];   //  68

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_spcr) && alignof (Acpi_table_spcr) == 1 && sizeof (Acpi_table_spcr) == 80);
