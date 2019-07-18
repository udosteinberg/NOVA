/*
 * Space
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

#include "types.hpp"

class Space
{
    public:
        enum class Type : uint8
        {
            OBJ     = 0,
            MEM     = 1,
            PIO     = 2,
            MSR     = 3,
        };

        enum class Index : uint8
        {
            CPU_HST = 0,
            CPU_GST = 1,
            DMA_HST = 2,
            DMA_GST = 3,
        };

        static inline bool is_gst (Index i) { return static_cast<uint8>(i) & 1; }
        static inline bool is_dma (Index i) { return static_cast<uint8>(i) & 2; }
};
