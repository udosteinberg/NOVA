/*
 * Memory Attributes: x86
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "std.hpp"
#include "types.hpp"

class Memattr final
{
    private:
        uint32_t val;

        static constexpr uint64_t pat { PAT_VAL };
        static constexpr uint64_t rev { PAT_ENC(CA_TYPE_MEM_WB, CA_MEM_WB) |
                                        PAT_ENC(CA_TYPE_MEM_WT, CA_MEM_WT) |
                                        PAT_ENC(CA_TYPE_MEM_WC, CA_MEM_WC) |
                                        PAT_ENC(CA_TYPE_MEM_UC, CA_MEM_UC) |
                                        PAT_ENC(CA_TYPE_MEM_WP, CA_MEM_WP) };

    public:
        static inline unsigned obits { 36 };
        static inline unsigned kbits {  0 };
        static inline unsigned kimax {  0 };

        // Key Identifier, encoded in [17:3]
        using Keyid = uint16_t;

        // Cacheability, encoded in [2:0]
        enum class Cache : unsigned
        {
            MEM_WB  = CA_MEM_WB,
            MEM_WT  = CA_MEM_WT,
            MEM_WC  = CA_MEM_WC,
            MEM_UC  = CA_MEM_UC,
            MEM_WP  = CA_MEM_WP,
            UNUSED,
        };

        Memattr() = default;
        explicit constexpr Memattr (uint32_t v) : val (v) {}
        explicit constexpr Memattr (Keyid ki, Cache ca) : val (ki << 3 | std::to_underlying (ca)) {}

        static constexpr auto ram() { return Memattr { 0, Cache::MEM_WB }; }
        static constexpr auto dev() { return Memattr { 0, Cache::MEM_UC }; }

        auto keyid() const { return BIT_RANGE (14, 0) & val >> 3; }

        auto cache_s1() const { return BIT_RANGE (2, 0) & val; }
        auto cache_s2() const { return BIT_RANGE (2, 0) & pat >> 8 * cache_s1(); }

        auto key_encode() const { return static_cast<uint64_t>(keyid()) << obits; }
        static auto key_decode (uint64_t val) { return Keyid (val >> obits & (BIT (kbits) - 1)); }

        static auto ept_to_ca (unsigned e) { return Cache (BIT_RANGE (2, 0) & rev >> 8 * e); }

        bool valid() const { return keyid() <= kimax && cache_s1() < std::to_underlying (Cache::UNUSED); }
};

#endif
