/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "acpi_gas.hpp"
#include "assert.hpp"
#include "io.hpp"
#include "lowlevel.hpp"
#include "macros.hpp"
#include "txt.hpp"

class Acpi_fixed final
{
    friend class Acpi_table_fadt;

    private:
        /*
         * 4.8.3: Power Management Timer (3.579545 MHz)
         */
        static constexpr auto timer_frequency { 3'579'545 };

        /*
         * 4.8.4: Fixed Hardware Registers
         */
        enum class Register : unsigned
        {
            PM1_STS,        // PM1 Status
            PM1_ENA,        // PM1 Enable
            PM1_CNT,        // PM1 Control
            PM2_CNT,        // PM2 Control
            PM_TMR,         // PM Timer
            RST_REG,        // Reset
            SLP_CNT,        // Sleep Control
            SLP_STS,        // Sleep Status
        };

        static inline constinit Acpi_gas gpe0_sts, gpe0_ena, gpe1_sts, gpe1_ena, pm1a_sts, pm1a_ena, pm1b_sts, pm1b_ena, pm1a_cnt, pm1b_cnt, pm2_cnt, pm_tmr, rst_reg, slp_cnt, slp_sts;
        static inline constinit unsigned gpe0_len { 0 }, gpe1_len { 0 };
        static inline constinit uint8_t rst_val { 0 };

        NOINLINE
        static unsigned read (Acpi_gas &g)
        {
            if (g.asid == Acpi_gas::Asid::PIO) {
                switch (g.bits) {
                    case  0: return 0;  // Register non-existent
                    case  8: return Io::in<uint8_t> (static_cast<port_t>(g.addr));
                    case 16: return Io::in<uint16_t>(static_cast<port_t>(g.addr));
                    case 32: return Io::in<uint32_t>(static_cast<port_t>(g.addr));
                }
            }

            return 0;
        }

        NOINLINE
        static void write (Acpi_gas &g, unsigned v)
        {
            if (g.asid == Acpi_gas::Asid::PIO) {
                switch (g.bits) {
                    case  0: return;    // Register non-existent
                    case  8: return Io::out<uint8_t> (static_cast<port_t>(g.addr), static_cast<uint8_t> (v));
                    case 16: return Io::out<uint16_t>(static_cast<port_t>(g.addr), static_cast<uint16_t>(v));
                    case 32: return Io::out<uint32_t>(static_cast<port_t>(g.addr), static_cast<uint32_t>(v));
                }
            }
        }

        /*
         * Write to a GPE register
         *
         * @param g     Generic address structure (5.2.9: bits, offs, accs must be ignored)
         * @param l     Length of the register in bytes (5.2.9: length is always GPEx_BLK_LEN / 2)
         * @param v     Value to write (4.8.4.1: each register in the block is accessed as a byte)
         */
        NOINLINE
        static void write_gpe (Acpi_gas &g, unsigned l, uint8_t v)
        {
            if (g.asid == Acpi_gas::Asid::PIO)
                for (unsigned i { 0 }; i < l; i++)
                    Io::out<uint8_t>(static_cast<port_t>(g.addr + i), v);
        }

        ALWAYS_INLINE
        static inline unsigned read (Register r)
        {
            switch (r) {
                case Register::PM1_STS: return read (pm1a_sts) | read (pm1b_sts);
                case Register::PM1_ENA: return read (pm1a_ena) | read (pm1b_ena);
                case Register::PM1_CNT: return read (pm1a_cnt) | read (pm1b_cnt);
                case Register::PM2_CNT: return read (pm2_cnt);
                case Register::PM_TMR:  return read (pm_tmr);
                default:                return 0;
            }
        }

        ALWAYS_INLINE
        static inline void write (Register r, unsigned v)
        {
            switch (r) {
                case Register::PM1_STS: write (pm1a_sts, v); write (pm1b_sts, v); break;
                case Register::PM1_ENA: write (pm1a_ena, v); write (pm1b_ena, v); break;
                case Register::PM1_CNT: write (pm1a_cnt, v); write (pm1b_cnt, v); break;
                case Register::PM2_CNT: write (pm2_cnt,  v);                      break;
                case Register::RST_REG: write (rst_reg,  v);                      break;
                default:                /* read-only */                           break;
            }
        }

        static bool enabled()
        {
            return read (Register::PM1_CNT) & BIT (0);
        }

        static bool can_reset() { return rst_reg.valid(); }
        static bool can_sleep() { return (slp_cnt.valid() || pm1a_cnt.valid()) && (slp_sts.valid() || pm1a_sts.valid()); }

    public:
        class Transition final
        {
            private:
                uint16_t val { 0 };

            public:
                bool valid() const { return BIT (15)         & val; }
                auto state() const { return BIT_RANGE (2, 0) & val; }
                auto val_a() const { return BIT_RANGE (2, 0) & val >> 3; }
                auto val_b() const { return BIT_RANGE (2, 0) & val >> 6; }

                explicit constexpr Transition() = default;
                explicit constexpr Transition (uint16_t v) : val { v } {}
        };

        static bool supported (Transition t)
        {
            return ((BIT_RANGE (5, 3) | BIT (1)) & BIT (t.state()) && can_sleep()) || (BIT (0) & BIT (t.state()) && can_reset());
        }

        /*
         * Offline the calling core
         */
        static bool offline_core() { return true; }

        /*
         * Wait for all APs to be offline
         */
        static void offline_wait() { Txt::fini(); }

        /*
         * Perform platform reset
         */
        static bool reset()
        {
            assert (can_reset());

            write (Register::RST_REG, rst_val);

            return false;
        }

        /*
         * Perform platform sleep
         */
        static bool sleep (Transition t)
        {
            assert (can_sleep());

            if (slp_cnt.valid()) {
                auto const v { (read (Register::SLP_CNT) | BIT (5)) & ~BIT_RANGE (4, 2) };
                write (Register::SLP_CNT, v | t.val_a() << 2);
            } else {
                auto const v { (read (Register::PM1_CNT) | BIT (13)) & ~BIT_RANGE (12, 10) };
                write (pm1a_cnt, v | t.val_a() << 10);
                write (pm1b_cnt, v | t.val_b() << 10);
            }

            return false;
        }

        /*
         * Clear wake bits
         */
        static void wake_clr()
        {
            assert (can_sleep());

            if (slp_sts.valid())
                write (Register::SLP_STS, BIT (7));
            else {
                write (Register::PM1_ENA, 0);
                write (Register::PM1_STS, BIT (15) | (read (Register::PM1_STS) & BIT_RANGE (10, 8)));

                write_gpe (gpe0_ena, gpe0_len, 0);          // clear enable bits
                write_gpe (gpe0_sts, gpe0_len, 0xff);       // clear status bits
                write_gpe (gpe1_ena, gpe1_len, 0);          // clear enable bits
                write_gpe (gpe1_sts, gpe1_len, 0xff);       // clear status bits
            }
        }

        /*
         * Poll wake status bit
         */
        static void wake_chk()
        {
            assert (can_sleep());

            if (slp_sts.valid())
                for (; !(read (Register::SLP_STS) & BIT (7)); pause()) ;
            else
                for (; !(read (Register::PM1_STS) & BIT (15)); pause()) ;
        }

        static void delay (unsigned ms)
        {
            unsigned const cnt { timer_frequency * ms / 1000 };
            unsigned const val { read (Register::PM_TMR) };

            while ((read (Register::PM_TMR) - val) % BIT (24) < cnt)
                pause();
        }

        /*
         * Enable ACPI mode
         */
        static void enable (port_t scp, uint8_t e, uint8_t p, uint8_t c)
        {
            assert (scp);

            if (e && !enabled())        // OSPM ACPI HW Control
                for (Io::out (scp, e); !enabled(); pause()) ;

            if (p)                      // OSPM P-State Control
                Io::out (scp, p);

            if (c)                      // OSPM C-State Control
                Io::out (scp, c);
        }
};
