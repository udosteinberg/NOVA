/*
 * Debug Port Types
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

class Debug
{
    public:
        enum class Type : uint16
        {
            SERIAL                  = 0x8000,
            FIREWIRE                = 0x8001,
            USB                     = 0x8002,
            NET                     = 0x8003,
        };

        enum class Subtype : uint16
        {
            SERIAL_NS16550_FULL     = 0x0000,   // 16550 fully compatible
            SERIAL_NS16550_SUBSET   = 0x0001,   // 16550 subset compatible with DBGP Revision 1
            SERIAL_MAX311xE         = 0x0002,   // MAX311xE SPI UART
            SERIAL_PL011            = 0x0003,   // ARM PL011 UART
            SERIAL_MSM8x60          = 0x0004,   // MSM8x60 (e.g. 8960)
            SERIAL_NS16550_NVIDIA   = 0x0005,   // Nvidia 16550
            SERIAL_OMAP             = 0x0006,   // TI OMAP
            SERIAL_APM88xxxx        = 0x0008,   // APM88xxxx
            SERIAL_MSM8974          = 0x0009,   // MSM8974
            SERIAL_SAM5250          = 0x000a,   // SAM5250
            SERIAL_USIF             = 0x000b,   // Intel USIF
            SERIAL_IMX              = 0x000c,   // i.MX 6
            SERIAL_SBSA_2x          = 0x000d,   // ARM SBSA Generic UART (2.x only) supporting only 32-bit accesses
            SERIAL_SBSA             = 0x000e,   // ARM SBSA Generic UART
            SERIAL_DCC              = 0x000f,   // ARM DCC
            SERIAL_BCM2835          = 0x0010,   // BCM2835
            SERIAL_SDM845_18        = 0x0011,   // SDM845 with clock rate of 1.8432 MHz
            SERIAL_NS16550_PARAM    = 0x0012,   // 16550-compatible with parameters defined in Generic Address Structure
            SERIAL_SDM845_73        = 0x0013,   // SDM845 with clock rate of 7.372 MHz
            SERIAL_LPSS             = 0x0014,   // Intel LPSS
        };
};
