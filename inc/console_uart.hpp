/*
 * UART Console
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "cmdline.hpp"
#include "console_mbuf.hpp"
#include "ptab_hpt.hpp"
#include "wait.hpp"

class Console_uart : protected Console
{
    private:
        // Transmission of a character should take 86.6µs at 115200 baud, but some network consoles take much longer
        static constexpr unsigned timeout { 5000 };

        static inline constinit Atomic<uintptr_t> mmap_base { MMAP_GLB_UART };

        virtual void tx (uint8_t) const = 0;

        [[nodiscard]] virtual bool tx_busy() const = 0;
        [[nodiscard]] virtual bool tx_full() const = 0;

        [[nodiscard]] bool fini() const override final
        {
            Wait::until (timeout, [&] { return !tx_busy(); });

            return false;
        }

        [[nodiscard]] bool outc (char c) const override final
        {
            if (!Wait::until (timeout, [&] { return !tx_full(); })) [[unlikely]]
                return false;

            tx (c);

            return true;
        }

        void sync()
        {
            auto const mbuf { Console_mbuf::singleton.regs };

            if (mbuf) [[likely]]
                for (uint32_t r { mbuf->r_idx }, w { mbuf->w_idx }; r != w; r = (r + 1) % mbuf->entries)
                    if (!outc (mbuf->buffer[r])) [[unlikely]]
                        return;

            enable();
        }

        [[nodiscard]] bool using_regs (Acpi_gas const &r) const override final
        {
            return (r.asid == Acpi_gas::Asid::MEM && r.addr == regs.mem) || (r.asid == Acpi_gas::Asid::PIO && r.addr == regs.pio);
        }

    protected:
        static constexpr unsigned baudrate { 115'200 };

        unsigned const clock;
        uintptr_t mmap { 0 };

        struct Regs final
        {
            uint64_t    mem { 0 };
            uint16_t    pio { 0 };
            uint8_t     shl { 0 };
        } regs;

        bool setup (Regs const r)
        {
            if (Cmdline::nouart || (!r.mem && !r.pio)) [[unlikely]]
                return false;

            if (r.mem) [[likely]] {
                mmap = mmap_base.fetch_add (PAGE_SIZE (0)) | (r.mem & OFFS_MASK (0));
                Hptp::master_map (mmap & ~OFFS_MASK (0), r.mem & ~OFFS_MASK (0), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());
            }

            regs = r;

            if (init()) [[likely]]
                sync();

            return true;
        }

        [[nodiscard]] bool setup_regs (Acpi_gas const &r) override final
        {
            return setup (Regs { .mem = (r.asid == Acpi_gas::Asid::MEM) * r.addr, .pio = static_cast<port_t>((r.asid == Acpi_gas::Asid::PIO) * r.addr), .shl = static_cast<uint8_t>(bit_scan_lsb (r.bits) - 3) });
        }

        explicit Console_uart (unsigned c) : clock { c } {}
};
