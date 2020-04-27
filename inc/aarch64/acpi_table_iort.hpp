/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "acpi_table.hpp"

#pragma pack(1)

/*
 * I/O Remapping Table (IORT)
 */
class Acpi_table_iort final
{
    private:
        Acpi_table  table;                          //  0
        uint32      node_cnt;                       // 36
        uint32      node_ofs;                       // 40
        uint32      reserved;                       // 44

        struct Node
        {
            enum class Type : uint8
            {
                ITS_GROUP       = 0,
                NAMED_COMPONENT = 1,
                ROOT_COMPLEX    = 2,
                SMMUv1v2        = 3,
                SMMUv3          = 4,
                PMCG            = 5,
                MEM_RANGE       = 6,
            };

            Type    type;                           //  0
            uint16  length;                         //  1
            uint8   rev;                            //  3
            uint16  reserved;                       //  4
            uint16  id;                             //  6
            uint32  cnt;                            //  8
            uint32  ofs;                            // 12
        };

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_iort) && sizeof (Acpi_table_iort) == 48);

#pragma pack()
