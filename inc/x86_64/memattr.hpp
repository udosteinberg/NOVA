/*
 * Memory Attributes: x86
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "macros.hpp"

#define CA_TYPE_MEM_UC  0
#define CA_TYPE_MEM_WC  1
#define CA_TYPE_MEM_WT  4
#define CA_TYPE_MEM_WP  5
#define CA_TYPE_MEM_WB  6

/*
 * Memory type for accesses to a page table pointed to by CR3:
 * - if CR4.PCIDE=1, hardwired to PAT[0], therefore PAT[0] must be WB
 * - if CR4.PCIDE=0, based on CR3[PCD/PWT]
 *
 * Memory type for accesses to a page table pointed to by a PTE:
 * - based on PTE[PCD/PWT]
 *
 * Memory type for accesses to a leaf page pointed to by a PTE:
 * - based on PTE[PAT/PCD/PWT]
 */
                            // PAT PCD PWT
#define CA_MEM_WB       0   //  0   0   0
#define CA_MEM_WT       1   //  0   0   1
#define CA_MEM_WC       2   //  0   1   0
#define CA_MEM_UC       3   //  0   1   1
#define CA_MEM_WP       4   //  1   0   0

#define PAT_ENC(CA, T)  (SFX_ULL(T) << ((CA) << 3))

#define PAT_VAL         (PAT_ENC(CA_MEM_WB, CA_TYPE_MEM_WB) |   \
                         PAT_ENC(CA_MEM_WT, CA_TYPE_MEM_WT) |   \
                         PAT_ENC(CA_MEM_WC, CA_TYPE_MEM_WC) |   \
                         PAT_ENC(CA_MEM_UC, CA_TYPE_MEM_UC) |   \
                         PAT_ENC(CA_MEM_WP, CA_TYPE_MEM_WP))

#ifndef __ASSEMBLER__

#include "types.hpp"

class Memattr
{
    private:
        static constexpr uint64 pat { PAT_VAL };
        static constexpr uint64 rev { PAT_ENC(CA_TYPE_MEM_WB, CA_MEM_WB) |
                                      PAT_ENC(CA_TYPE_MEM_WT, CA_MEM_WT) |
                                      PAT_ENC(CA_TYPE_MEM_WC, CA_MEM_WC) |
                                      PAT_ENC(CA_TYPE_MEM_UC, CA_MEM_UC) |
                                      PAT_ENC(CA_TYPE_MEM_WP, CA_MEM_WP) };

    public:
        enum class Cacheability
        {
            MEM_WB  = CA_MEM_WB,
            MEM_WT  = CA_MEM_WT,
            MEM_WC  = CA_MEM_WC,
            MEM_UC  = CA_MEM_UC,
            MEM_WP  = CA_MEM_WP,
        };

        enum class Shareability
        {
            NONE    = 0,
            OUTER   = 0,
            INNER   = 0,
        };

        static unsigned ca_to_ept (Cacheability c) { return pat >> 8 * static_cast<unsigned>(c) & 0x7; }

        static Cacheability ept_to_ca (unsigned e) { return Cacheability (rev >> 8 * e & 0x7); }
};

#endif
