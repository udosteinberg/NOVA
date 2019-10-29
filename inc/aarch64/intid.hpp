/*
 * Interrupt Identifier
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

class Intid
{
    protected:
        enum
        {
            BASE_SGI =    0,
            BASE_PPI =   16,
            BASE_SPI =   32,
            BASE_RSV = 1020,
        };

    public:
        enum
        {
            NUM_SGI = BASE_PPI - BASE_SGI,
            NUM_PPI = BASE_SPI - BASE_PPI,
            NUM_SPI = BASE_RSV - BASE_SPI,
        };
};
