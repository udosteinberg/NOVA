/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_table_spcr.hpp"
#include "console_uart_mmio_imx.hpp"
#include "console_uart_mmio_ns16550.hpp"
#include "console_uart_mmio_pl011.hpp"

void Acpi_table_spcr::parse() const
{
    // Only accept MMIO registers
    if (regs.asid != Acpi_gas::Asid::MMIO)
        return;

    switch (uart_type) {

        case Subtype::SERIAL_PL011:
            Console_uart_mmio_pl011::singleton.mmap (regs.addr);
            break;

        case Subtype::SERIAL_IMX:
            Console_uart_mmio_imx::singleton.mmap (regs.addr);
            break;

        case Subtype::SERIAL_NS16550_FULL:
        case Subtype::SERIAL_NS16550_SUBSET:
        case Subtype::SERIAL_NS16550_PARAM:
            Console_uart_mmio_ns16550::singleton.mmap (regs.addr);
            break;

        default:
            break;
    }
}
