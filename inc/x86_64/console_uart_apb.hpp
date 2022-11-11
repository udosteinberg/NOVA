/*
 * Console: Advanced Peripheral Bus (APB)
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

#include "console_uart_ns16550.hpp"
#include "pci.hpp"

class Console_uart_apb final : private Console_uart_ns16550
{
    private:
        static Console_uart_apb uart0, uart1, uart2;

        // Null-Terminated BDF List
        static constexpr pci_t bdf0[] { Pci::bdf (0, 30, 0), Pci::bdf (0, 24, 0), 0 };
        static constexpr pci_t bdf1[] { Pci::bdf (0, 30, 1), Pci::bdf (0, 24, 1), 0 };
        static constexpr pci_t bdf2[] { Pci::bdf (0, 25, 2), Pci::bdf (0, 25, 0), Pci::bdf (0, 24, 2), 0 };

        static_assert (!bdf0[sizeof (bdf0) / sizeof (*bdf0) - 1]);
        static_assert (!bdf1[sizeof (bdf1) / sizeof (*bdf1) - 1]);
        static_assert (!bdf2[sizeof (bdf2) / sizeof (*bdf2) - 1]);

        // Null-Terminated DID List:       EBG     MTL-S   MTL-U   RPL-S   ADL-S   ADL-N   ADL-U   TGP-H   TGP-LP  ICP-LP  CMP-H   CMP-LP  CMP-V   CNP-H   CNP-LP  KBP-H   SPT-H   SPT-LP  JSL     EHL     GLK     APL
        static constexpr uint16_t did0[] { 0x1bad, 0x7f28, 0x7e25, 0x7a28, 0x7aa8, 0x54a8, 0x51a8, 0x43a8, 0xa0a8, 0x34a8, 0x06a8, 0x02a8, 0xa3a7, 0xa328, 0x9da8, 0xa2a7, 0xa127, 0x9d27, 0x4da8, 0x4b28, 0x31bc, 0x5abc, 0 };
        static constexpr uint16_t did1[] { 0x1bae, 0x7f29, 0x7e26, 0x7a29, 0x7aa9, 0x54a9, 0x51a9, 0x43a9, 0xa0a9, 0x34a9, 0x06a9, 0x02a9, 0xa3a8, 0xa329, 0x9da9, 0xa2a8, 0xa128, 0x9d28, 0x4da9, 0x4b29, 0x31be, 0x5abe, 0 };
        static constexpr uint16_t did2[] { /*n/a*/ 0x7f5c, 0x7e52, 0x7a7e, 0x7afe, 0x54c7, 0x51c7, 0x43a7, 0xa0c7, 0x34c7, 0x06c7, 0x02c7, 0xa3e6, 0xa347, 0x9dc7, 0xa2e6, 0xa166, 0x9d66, 0x4dc7, 0x4b4d, 0x31c0, 0x5ac0, 0 };

        static_assert (!did0[sizeof (did0) / sizeof (*did0) - 1]);
        static_assert (!did1[sizeof (did1) / sizeof (*did1) - 1]);
        static_assert (!did2[sizeof (did2) / sizeof (*did2) - 1]);

        static Regs probe (pci_t const[], uint16_t const[]);

    protected:
        explicit Console_uart_apb (pci_t const bdf[], uint16_t const did[]) : Console_uart_ns16550 (probe (bdf, did), 1'843'200) {}
};
