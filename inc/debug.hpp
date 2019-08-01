/*
 * Debug Port Types
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

#include "types.hpp"

class Debug
{
    public:
        enum class Type : uint16_t
        {
            SERIAL                  = 0x8000,
            FIREWIRE                = 0x8001,
            USB                     = 0x8002,
            NET                     = 0x8003,
        };

        /*
         * There are three interface subtypes which can be used for 16550-based UARTs. The differences between them are subtle yet important.
         * Subtype 0x0 refers to a serial port which uses "legacy" port I/O as seen on x86-based platforms. This type should be avoided on platforms that use memory-mapped I/O, such as ARM or RISC-V.
         * Subtype 0x1 supports memory mapped UARTs, but only ones that are describable in the DBGP ACPI table. Operating system implementations may treat this as equivalent to a DBGP-provided debug port and honor only the Base Address field of the Generic Address Structure.
         * Subtype 0x12 is the most flexible choice and is recommended when running compatible operating systems on new platforms. This subtype supports all serial ports which can be described by the subtypes 0x0 and 0x1, as well as new ones, such as those requiring non-traditional access sizes and bit widths in the Generic Address Structure.
         */
        enum class Subtype : uint16_t
        {
            SERIAL_NS16550_LEGACY   = 0x0000,   // 16550 fully compatible
            SERIAL_NS16550_DBGP     = 0x0001,   // 16550 subset compatible with DBGP Revision 1
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
            SERIAL_BCM2835          = 0x0010,   // BCM2835 (MiniUART)
            SERIAL_SDM845_18        = 0x0011,   // SDM845 with clock rate of 1.8432 MHz
            SERIAL_NS16550_DBG2     = 0x0012,   // 16550-compatible with parameters defined in Generic Address Structure
            SERIAL_SDM845_73        = 0x0013,   // SDM845 with clock rate of 7.372 MHz
            SERIAL_LPSS             = 0x0014,   // Intel LPSS
            SERIAL_RISCV_SBI        = 0x0015,   // RISC-V SBI

            SERIAL_CDNS             = 0xfffb,
            SERIAL_GENI             = 0xfffc,
            SERIAL_MESON            = 0xfffd,
            SERIAL_SCIF             = 0xfffe,
            SERIAL_NONE             = 0xffff,

            USB_XHCI                = 0x0000,   // XHCI-compliant controller with debug interface
            USB_EHCI                = 0x0001,   // EHCI-compliant controller with debug interface
        };
};
