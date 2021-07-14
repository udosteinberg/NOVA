/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
 * HPET Description Table (HPET)
 */
class Acpi_table_hpet final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      cap;                        // 36
        Acpi_gas                    regs;                       // 40
        Unaligned_le<uint8_t>       uid;                        // 52
        Unaligned_le<uint16_t>      tick;                       // 53
        Unaligned_le<uint8_t>       attr;                       // 55

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_hpet) && alignof (Acpi_table_hpet) == 1 && sizeof (Acpi_table_hpet) == 56);
