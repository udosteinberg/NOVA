/*
 * Advanced Configuration and Power Interface (ACPI)
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
#include "acpi_table.hpp"

#pragma pack(1)

/*
 * HPET Description Table (HPET)
 */
class Acpi_table_hpet final
{
    private:
        Acpi_table  table;                              //  0
        uint32      cap;                                // 36
        Acpi_gas    hpet;                               // 40
        uint8       acpi_uid;                           // 52
        uint16      tick;                               // 53 (unaligned)
        uint8       attr;                               // 55

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_hpet) && sizeof (Acpi_table_hpet) == 56);

#pragma pack()
