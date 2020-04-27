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
 * I/O Remapping Table (IORT)
 */
class Acpi_table_iort final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      node_cnt;                   // 36
        Unaligned_le<uint32_t>      node_ofs;                   // 40
        Unaligned_le<uint32_t>      reserved;                   // 44

        struct Node : private Unaligned_le<uint8_t>             // 0
        {
            Unaligned_le<uint16_t>  length;                     // 1
            Unaligned_le<uint8_t>   rev;                        // 3
            Unaligned_le<uint16_t>  reserved;                   // 4
            Unaligned_le<uint16_t>  id;                         // 6
            Unaligned_le<uint32_t>  cnt;                        // 8
            Unaligned_le<uint32_t>  ofs;                        // 12

            enum class Type : uint8_t
            {
                ITS_GROUP       = 0,
                NAMED_COMPONENT = 1,
                ROOT_COMPLEX    = 2,
                SMMUv1v2        = 3,
                SMMUv3          = 4,
                PMCG            = 5,
                MEM_RANGE       = 6,
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Node) == 1 && sizeof (Node) == 16);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_iort) && alignof (Acpi_table_iort) == 1 && sizeof (Acpi_table_iort) == 48);
