/*
 * Event Selectors
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

class Event final
{
    public:
        enum Selector
        {
            NONE        = -1,
            STARTUP     =  0,
            RECALL      =  1,
            VTIMER      =  2,
        };

        static constexpr auto hst_arch  { 64 };
        static constexpr auto gst_arch  { 64 };

        static constexpr auto hst_max   { 1 + RECALL };
        static constexpr auto gst_max   { 1 + VTIMER };
};
