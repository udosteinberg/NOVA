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
            Unaligned_le<uint16_t>  len;                        // 1
            Unaligned_le<uint8_t>   rev;                        // 3
            Unaligned_le<uint32_t>  uid;                        // 4
            Unaligned_le<uint32_t>  cnt;                        // 8
            Unaligned_le<uint32_t>  ofs;                        // 12

            enum class Type : uint8_t
            {
                ITS_GROUP       = 0,                            // ITS Group
                NAMED_COMPONENT = 1,                            // Named Component
                ROOT_COMPLEX    = 2,                            // Root Complex
                SMMU_V2         = 3,                            // SMMUv1 or SMMUv2
                SMMU_V3         = 4,                            // SMMUv3
                PMCG            = 5,                            // PMCG
                MEM_RANGE       = 6,                            // Memory Range
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Node) == 1 && sizeof (Node) == 16);

        /*
         * 3.1.1.1 SMMUv1 or SMMUv2
         */
        struct Node_smmu_v2 : public Node
        {
            Unaligned_le<uint64_t>  base;                       // 16
            Unaligned_le<uint64_t>  span;                       // 24
            Unaligned_le<uint32_t>  model;                      // 32
            Unaligned_le<uint32_t>  flags;                      // 36
            Unaligned_le<uint32_t>  glb_ofs;                    // 40
            Unaligned_le<uint32_t>  ctx_cnt;                    // 44
            Unaligned_le<uint32_t>  ctx_ofs;                    // 48
            Unaligned_le<uint32_t>  pmu_cnt;                    // 52
            Unaligned_le<uint32_t>  pmu_ofs;                    // 56

            void parse() const;
        };

        static_assert (alignof (Node_smmu_v2) == 1 && sizeof (Node_smmu_v2) == 60);

        /*
         * 3.1.1.2 SMMUv3
         */
        struct Node_smmu_v3 : public Node
        {
            Unaligned_le<uint64_t>  base;                       // 16
            Unaligned_le<uint32_t>  flags;                      // 24
            Unaligned_le<uint32_t>  res;                        // 28
            Unaligned_le<uint64_t>  vatos;                      // 32
            Unaligned_le<uint32_t>  model;                      // 40
            Unaligned_le<uint32_t>  intid_e;                    // 44
            Unaligned_le<uint32_t>  intid_p;                    // 48
            Unaligned_le<uint32_t>  intid_g;                    // 52
            Unaligned_le<uint32_t>  intid_s;                    // 56
            Unaligned_le<uint32_t>  pxd;                        // 60
            Unaligned_le<uint32_t>  midx;                       // 64

            void parse() const;
        };

        static_assert (alignof (Node_smmu_v3) == 1 && sizeof (Node_smmu_v3) == 68);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_iort) && alignof (Acpi_table_iort) == 1 && sizeof (Acpi_table_iort) == 48);
