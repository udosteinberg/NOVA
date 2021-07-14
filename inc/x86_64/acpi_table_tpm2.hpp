/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
 * TCG Hardware Interface Description Table for TPM 2.0 (TPM2)
 */
class Acpi_table_tpm2 final
{
    private:
        Acpi_table          table;                  //  0
        uint16_t            platform;               // 36
        uint16_t            reserved;               // 38
        uint64_t            tpm_base;               // 40
        uint32_t            start_method;           // 48
        uint32_t            start_params;           // 52

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_tpm2) && sizeof (Acpi_table_tpm2) == 56);
