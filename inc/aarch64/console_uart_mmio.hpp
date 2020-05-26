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
        static inline uintptr_t mmap_base { DEV_GLOBL_UART };

    protected:
        uintptr_t mmio_base { 0 };

        inline explicit Console_uart_mmio (unsigned c) : Console_uart (c) {}

    public:
        void mmap (Hpt::OAddr p)
        {
            if (!p || mmio_base)
                return;

            // Advance memory map pointer
            mmio_base = __atomic_fetch_add (&mmap_base, PAGE_SIZE, __ATOMIC_SEQ_CST);

            Hptp::master.update (mmio_base, p, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

            init();

            enable();
        }
};
