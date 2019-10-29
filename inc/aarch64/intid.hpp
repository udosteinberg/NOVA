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
            SGI_BASE =    0,
            PPI_BASE =   16,
            SPI_BASE =   32,
            RSV_BASE = 1020,
        };

    public:
        enum
        {
            SGI_NUM  = PPI_BASE - SGI_BASE,
            PPI_NUM  = SPI_BASE - PPI_BASE,
            SPI_NUM  = RSV_BASE - SPI_BASE,
        };
};
