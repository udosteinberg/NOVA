/*
 * Memory Attributes: ARM
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

#define SH_NONE         0
#define SH_OUTER        2
#define SH_INNER        3

#define MAIR_ENC(CA, T) (SFX_ULL(T) << ((CA) << 3))

#define MAIR_VAL        (MAIR_ENC(CA_DEV,       CA_TYPE_DEV)        |   \
                         MAIR_ENC(CA_DEV_E,     CA_TYPE_DEV_E)      |   \
                         MAIR_ENC(CA_DEV_RE,    CA_TYPE_DEV_RE)     |   \
                         MAIR_ENC(CA_DEV_GRE,   CA_TYPE_DEV_GRE)    |   \
                         MAIR_ENC(CA_MEM_NC,    CA_TYPE_MEM_NC)     |   \
                         MAIR_ENC(CA_MEM_WT,    CA_TYPE_MEM_WT)     |   \
                         MAIR_ENC(CA_MEM_WB,    CA_TYPE_MEM_WB))

#ifndef __ASSEMBLER__

#include "types.hpp"

class Memattr
{
    private:
        static constexpr uint64 mair = MAIR_VAL;

    public:
        enum class Cacheability
        {
            DEV     = CA_DEV,
            DEV_E   = CA_DEV_E,
            DEV_RE  = CA_DEV_RE,
            DEV_GRE = CA_DEV_GRE,
            MEM_NC  = CA_MEM_NC,
            MEM_WT  = CA_MEM_WT,
            MEM_WB  = CA_MEM_WB,
        };

        enum class Shareability
        {
            NONE    = SH_NONE,
            OUTER   = SH_OUTER,
            INNER   = SH_INNER,
        };

        static unsigned npt_attr (Cacheability ca)
        {
            unsigned attr = mair >> (static_cast<unsigned>(ca) << 3) & 0xff;
            return (attr >> 4 & 0xc) | (attr >> 2 & 0x3);
        }
};

#endif
