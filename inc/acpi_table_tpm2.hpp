/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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
        Acpi_table                  table;                      // 0
        Unaligned_le<uint16_t>      platform;                   // 36   4.0+
        Unaligned_le<uint16_t>      reserved;                   // 38   4.0+
        Unaligned_le<uint64_t>      ctrl_area;                  // 40   4.0+
        Unaligned_le<uint32_t>      start_method;               // 48   4.0+

        // Layout of subsequent fields differs between table versions

        // The specification states that all registers must be naturally aligned
        struct Control
        {
            Aligned_le<uint32_t> volatile   reserved;           // 0    Tpm::Crb::Reg32::CTRL_REQ
            Aligned_le<uint32_t> volatile   status;             // 4    Tpm::Crb::Reg32::CTRL_STS
            Aligned_le<uint32_t> volatile   cancel;             // 8
            Aligned_le<uint32_t> volatile   start;              // 12   Tpm::Crb::Reg32::CTRL_START
            Aligned_le<uint32_t> volatile   int_enable;         // 16
            Aligned_le<uint32_t> volatile   int_status;         // 20
            Aligned_le<uint32_t> volatile   cmd_size;           // 24
            Aligned_le<uint32_t> volatile   cmd_addr_lo;        // 28
            Aligned_le<uint32_t> volatile   cmd_addr_hi;        // 32
            Aligned_le<uint32_t> volatile   rsp_size;           // 36
            Aligned_le<uint64_t> volatile   rsp_addr;           // 40
        };

        static_assert (__is_standard_layout (Control) && alignof (Control) == 8 && sizeof (Control) == 48);

    public:
        [[nodiscard]] bool parse() const;
};

static_assert (__is_standard_layout (Acpi_table_tpm2) && alignof (Acpi_table_tpm2) == 1 && sizeof (Acpi_table_tpm2) == 52);
