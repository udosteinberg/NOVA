/*
 * Memory Attributes: ARM
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

#include "macros.hpp"

#define CA_TYPE_DEV     0x00    // Device
#define CA_TYPE_DEV_E   0x04    // Device, Early Ack
#define CA_TYPE_DEV_RE  0x08    // Device, Early Ack + Reordering
#define CA_TYPE_DEV_GRE 0x0c    // Device, Early Ack + Reordering + Gathering
#define CA_TYPE_MEM_NC  0x44    // Memory, Outer + Inner NC
#define CA_TYPE_MEM_WT  0xbb    // Memory, Outer + Inner WT Non-Transient
#define CA_TYPE_MEM_WB  0xff    // Memory, Outer + Inner WB Non-Transient

#define CA_DEV          0
#define CA_DEV_E        1
#define CA_DEV_RE       2
#define CA_DEV_GRE      3
#define CA_MEM_NC       5
#define CA_MEM_WT       6
#define CA_MEM_WB       7

#define MAIR_ENC(CA, T) (SFX_ULL(T) << ((CA) << 3))

#define MAIR_VAL        (MAIR_ENC(CA_DEV,       CA_TYPE_DEV)        |   \
                         MAIR_ENC(CA_DEV_E,     CA_TYPE_DEV_E)      |   \
                         MAIR_ENC(CA_DEV_RE,    CA_TYPE_DEV_RE)     |   \
                         MAIR_ENC(CA_DEV_GRE,   CA_TYPE_DEV_GRE)    |   \
                         MAIR_ENC(CA_MEM_NC,    CA_TYPE_MEM_NC)     |   \
                         MAIR_ENC(CA_MEM_WT,    CA_TYPE_MEM_WT)     |   \
                         MAIR_ENC(CA_MEM_WB,    CA_TYPE_MEM_WB))

#ifndef __ASSEMBLER__

#include "std.hpp"
#include "types.hpp"

class Memattr final
{
    private:
        uint32_t val;

        static constexpr uint64_t mair { MAIR_VAL };

    public:
        static constexpr unsigned obits { 48 };
        static constexpr unsigned kimax {  0 };

        // Shareability, encoded in [4:3]
        enum class Share : unsigned
        {
            NONE    = 0,
            RSVD    = 1,
            OUTER   = 2,
            INNER   = 3,
        };

        // Cacheability, encoded in [2:0]
        enum class Cache : unsigned
        {
            DEV     = CA_DEV,
            DEV_E   = CA_DEV_E,
            DEV_RE  = CA_DEV_RE,
            DEV_GRE = CA_DEV_GRE,
            RSVD    = 4,
            MEM_NC  = CA_MEM_NC,
            MEM_WT  = CA_MEM_WT,
            MEM_WB  = CA_MEM_WB,
        };

        Memattr() = default;
        explicit constexpr Memattr (uint32_t v) : val (v) {}
        explicit constexpr Memattr (Share sh, Cache ca) : val (std::to_underlying (sh) << 3 | std::to_underlying (ca)) {}

        static constexpr auto ram() { return Memattr { Share::INNER, Cache::MEM_WB }; }
        static constexpr auto dev() { return Memattr { Share::NONE,  Cache::DEV    }; }

        auto share() const { return BIT_RANGE (1, 0) & val >> 3; }

        auto cache_s1() const { return BIT_RANGE (2, 0) & val; }

        auto cache_s2() const
        {
            uint8_t attr = mair >> (cache_s1() << 3) & BIT_RANGE (7, 0);
            return (attr >> 4 & BIT_RANGE (3, 2)) | (attr >> 2 & BIT_RANGE (1, 0));
        }

        bool valid() const { return share() != std::to_underlying (Share::RSVD) && cache_s1() != std::to_underlying (Cache::RSVD); }
};

#endif
