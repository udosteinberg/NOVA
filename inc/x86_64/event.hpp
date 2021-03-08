/*
 * Event Selectors
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

class Event
{
    public:
        enum Selector
        {
            NONE        = -1,
            STARTUP     =  0,
            RECALL      =  1,
        };

        static constexpr unsigned hst_arch = 32;
        static constexpr unsigned gst_arch = 256;

        static constexpr unsigned hst_max  = 1 + RECALL;
        static constexpr unsigned gst_max  = 1 + RECALL;
};
