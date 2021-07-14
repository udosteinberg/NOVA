/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "acpi_gas.hpp"
#include "assert.hpp"
#include "io.hpp"
#include "lowlevel.hpp"
#include "macros.hpp"

class Acpi_fixed
{
    friend class Acpi_table_fadt;

    private:
        /*
         * 4.8.3: Power Management Timer (3.579545 MHz)
         */
        static constexpr unsigned timer_frequency = 3579545;

        /*
         * 4.8.4: Fixed Hardware Registers
         */
        enum class Register_fix
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

        /*
         * 4.8.5: Generic Hardware Registers
         */
        enum class Register_gen
        {
            GPE0_STS,       // General-Purpose Event 0 Status
            GPE0_ENA,       // General-Purpose Event 0 Enable
            GPE1_STS,       // General-Purpose Event 1 Status
            GPE1_ENA,       // General-Purpose Event 1 Enable
        };

        static inline Acpi_gas gpe0_sts, gpe0_ena, gpe1_sts, gpe1_ena, pm1a_sts, pm1a_ena, pm1b_sts, pm1b_ena, pm1a_cnt, pm1b_cnt, pm2_cnt, pm_tmr, rst_reg, slp_cnt, slp_sts;
        static inline uint8 rst_val;

        NOINLINE
        static unsigned read (Acpi_gas &g)
        {
            if (g.asid == Acpi_gas::Asid::PIO) {
                switch (g.bits) {
                    case  0: return 0;  // Register non-existent
                    case  8: return Io::in<uint8> (static_cast<unsigned>(g.addr));
                    case 16: return Io::in<uint16>(static_cast<unsigned>(g.addr));
                    case 32: return Io::in<uint32>(static_cast<unsigned>(g.addr));
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
                    case  8: return Io::out<uint8> (static_cast<unsigned>(g.addr), static_cast<uint8> (v));
                    case 16: return Io::out<uint16>(static_cast<unsigned>(g.addr), static_cast<uint16>(v));
                    case 32: return Io::out<uint32>(static_cast<unsigned>(g.addr), static_cast<uint32>(v));
                }
            }
        }

        NOINLINE
        static void write_block (Acpi_gas &g, uint8 v)
        {
            // 4.8.5.1: Each register in the block is accessed as a byte
            if (g.asid == Acpi_gas::Asid::PIO)
                for (unsigned i = 0; i < g.bits / 8; i++)
                    Io::out<uint8>(static_cast<unsigned>(g.addr) + i, v);
        }

        ALWAYS_INLINE
        static inline unsigned read (Register_fix r)
        {
            switch (r) {
                case Register_fix::PM1_STS: return read (pm1a_sts) | read (pm1b_sts);
                case Register_fix::PM1_ENA: return read (pm1a_ena) | read (pm1b_ena);
                case Register_fix::PM1_CNT: return read (pm1a_cnt) | read (pm1b_cnt);
                case Register_fix::PM2_CNT: return read (pm2_cnt);
                case Register_fix::PM_TMR:  return read (pm_tmr);
                default:                    return 0;
            }
        }

        ALWAYS_INLINE
        static inline void write (Register_fix r, unsigned v)
        {
            switch (r) {
                case Register_fix::PM1_STS: write (pm1a_sts, v); write (pm1b_sts, v); break;
                case Register_fix::PM1_ENA: write (pm1a_ena, v); write (pm1b_ena, v); break;
                case Register_fix::PM1_CNT: write (pm1a_cnt, v); write (pm1b_cnt, v); break;
                case Register_fix::PM2_CNT: write (pm2_cnt,  v);                      break;
                case Register_fix::RST_REG: write (rst_reg,  v);                      break;
                default:                    /* read-only */                           break;
            }
        }

        ALWAYS_INLINE
        static inline void clear (Register_gen r)
        {
            switch (r) {
                case Register_gen::GPE0_STS: return write_block (gpe0_sts, ~0);
                case Register_gen::GPE0_ENA: return write_block (gpe0_ena,  0);
                case Register_gen::GPE1_STS: return write_block (gpe1_sts, ~0);
                case Register_gen::GPE1_ENA: return write_block (gpe1_ena,  0);
            }
        }

        ALWAYS_INLINE
        static inline bool enabled()
        {
            return read (Register_fix::PM1_CNT) & BIT (0);
        }

        static inline bool can_reset() { return rst_reg.valid(); }
        static inline bool can_sleep() { return (slp_cnt.valid() || pm1a_cnt.valid()) && (slp_sts.valid() || pm1a_sts.valid()); }

    public:
        static inline bool supported (unsigned s)
        {
            switch (s) {
                case 1:
                case 5:
                    return can_sleep();
                case 7:
                    return can_reset();
                default:
                    return false;
            }
        }

        static void clear_events()
        {
            write (Register_fix::PM1_ENA, 0);

            clear (Register_gen::GPE0_ENA);
            clear (Register_gen::GPE1_ENA);
            clear (Register_gen::GPE0_STS);
            clear (Register_gen::GPE1_STS);
        }

        static inline void delay (unsigned ms)
        {
            unsigned cnt = timer_frequency * ms / 1000;
            unsigned val = read (Register_fix::PM_TMR);

            while ((read (Register_fix::PM_TMR) - val) % BIT (24) < cnt)
                pause();
        }

        /*
         * Perform platform reset
         */
        static inline void reset()
        {
            assert (can_reset());

            for (write (Register_fix::RST_REG, rst_val);; pause()) ;
        }

        /*
         * Perform platform sleep
         */
        static inline void sleep (unsigned a, unsigned b)
        {
            assert (can_sleep());
            assert (a < 8 && b < 8);

            if (slp_cnt.valid()) {
                auto v = (read (Register_fix::SLP_CNT) | BIT (5)) & ~BIT_RANGE (4, 2);
                write (Register_fix::SLP_CNT, v | a << 2);
            } else {
                auto v = (read (Register_fix::PM1_CNT) | BIT (13)) & ~BIT_RANGE (12, 10);
                write (pm1a_cnt, v | a << 10);
                write (pm1b_cnt, v | b << 10);
            }
        }

        /*
         * Clear wake bits
         */
        static inline void wake_clr()
        {
            assert (can_sleep());

            if (slp_sts.valid())
                write (Register_fix::SLP_STS, BIT (7));
            else
                write (Register_fix::PM1_STS, BIT (15) | (read (Register_fix::PM1_STS) & BIT_RANGE (10, 8)));
        }

        /*
         * Poll wake status bit
         */
        static inline void wake_chk()
        {
            assert (can_sleep());

            if (slp_sts.valid())
                for (; !(read (Register_fix::SLP_STS) & BIT (7)); pause()) ;
            else
                for (; !(read (Register_fix::PM1_STS) & BIT (15)); pause()) ;
        }

        /*
         * Enable ACPI mode
         */
        static void enable (unsigned port, uint8 e, uint8 c, uint8 p)
        {
            assert (port);

            if (e && !enabled())        // OSPM ACPI HW Control
                for (Io::out (port, e); !enabled(); pause()) ;

            if (c)                      // OSPM C-State Control
                Io::out (port, c);

            if (p)                      // OSPM P-State Control
                Io::out (port, p);
        }
};
