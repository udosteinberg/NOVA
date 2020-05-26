/*
 * UART Console (MMIO)
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

#include "console_uart.hpp"
#include "hpt.hpp"
#include "memory.hpp"

class Console_uart_mmio : protected Console_uart
{
    private:
        static uintptr_t mmap;

    protected:
        uintptr_t const mmio_base;

        Console_uart_mmio (Hpt::OAddr phys) : mmio_base (phys ? mmap : 0)
        {
            if (!mmio_base)
                return;

            Hptp::master.update (mmio_base, phys, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

            // Advance memory map pointer
            mmap += PAGE_SIZE;
        }
};
