/*
 * Space
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

class Space
{
    public:
        enum class Type
        {
            OBJ     = 0,
            MEM     = 1,
        };

        enum class Index
        {
            CPU_HST = 0,
            CPU_GST = 1,
            DMA_HST = 2,
            DMA_GST = 3,
        };

        static inline bool is_gst (Index i) { return unsigned (i) & 1; }
        static inline bool is_dma (Index i) { return unsigned (i) & 2; }
};
