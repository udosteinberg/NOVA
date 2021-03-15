/*
 * Local Advanced Programmable Interrupt Controller (LAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "cpu.hpp"
#include "lowlevel.hpp"

class Lapic final
{
    private:
        enum class Register32 : unsigned
        {
            IDR         = 0x2,              // Local APIC ID Register
            LVR         = 0x3,              // Local APIC Version Register
            TPR         = 0x8,              // Task Priority Register
            PPR         = 0xa,              // Processor Priority Register
            EOI         = 0xb,              // EOI Register
            LDR         = 0xd,              // Logical Destination Register
            DFR         = 0xe,              // Destination Format Register
            SVR         = 0xf,              // Spurious Vector Register
            ISR         = 0x10,             // In-Service Register
            TMR         = 0x18,             // Trigger Mode Register
            IRR         = 0x20,             // Interrupt Request Register
            ESR         = 0x28,             // Error Status Register
            ICR_LO      = 0x30,             // Interrupt Command Register [31:0]
            ICR_HI      = 0x31,             // Interrupt Command Register [63:32]
            LVT_TIMER   = 0x32,             // Local Vector Table: Timer
            LVT_THERM   = 0x33,             // Local Vector Table: Thermal Sensor
            LVT_PERFM   = 0x34,             // Local Vector Table: Performance Monitoring
            LVT_LINT0   = 0x35,             // Local Vector Table: Local Interrupt 0
            LVT_LINT1   = 0x36,             // Local Vector Table: Local Interrupt 1
            LVT_ERROR   = 0x37,             // Local Vector Table: Error Reporting
            TMR_ICR     = 0x38,             // Timer: Initial Count Register
            TMR_CCR     = 0x39,             // Timer: Current Count Register
            TMR_DCR     = 0x3e,             // Timer: Divide Configuration Register
            IPI_SELF    = 0x3f,             // Self-IPI Register (x2APIC only)
        };

        enum class Delivery : uint32_t
        {
            DLV_FIXED   = VAL_SHIFT (0, 8),
            DLV_NMI     = VAL_SHIFT (4, 8),
            DLV_INIT    = VAL_SHIFT (5, 8),
            DLV_SIPI    = VAL_SHIFT (6, 8),
            DLV_EXTINT  = VAL_SHIFT (7, 8),
        };

        /*
         * Read 32-bit register using the x2APIC (MSR) or Legacy (MMIO) interface
         *
         * @param r     LAPIC register
         * @return      LAPIC register value
         */
        static auto read_legacy (Register32 r) { return *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_APIC + (std::to_underlying (r) << 4)); }
        static auto read_x2apic (Register32 r) { return static_cast<uint32_t>(Msr::read (Msr::Array::IA32_X2APIC, 1, std::to_underlying (r))); }
        static auto read        (Register32 r) { return EXPECT_TRUE (x2apic) ? read_x2apic (r) : read_legacy (r); }

        /*
         * Write 32-bit register using the x2APIC (MSR) or Legacy (MMIO) interface
         *
         * @param r     LAPIC register
         * @param v     LAPIC register value
         */
        static void write_legacy (Register32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_APIC + (std::to_underlying (r) << 4)) = v; }
        static void write_x2apic (Register32 r, uint32_t v) { Msr::write (Msr::Array::IA32_X2APIC, 1, std::to_underlying (r), v); }
        static void write        (Register32 r, uint32_t v) { EXPECT_TRUE (x2apic) ? write_x2apic (r, v) : write_legacy (r, v); }

        /*
         * Write Interrupt Command Register
         *
         * @param v     LAPIC register value
         */
        static void set_icr (uint64_t v)
        {
            if (EXPECT_TRUE (x2apic))
                Msr::write (Msr::Array::IA32_X2APIC, 1, std::to_underlying (Register32::ICR_LO), v);

            else {
                while (EXPECT_FALSE (read_legacy (Register32::ICR_LO) & BIT (12)))
                    pause();

                write_legacy (Register32::ICR_HI, static_cast<uint32_t>(v >> 32));
                write_legacy (Register32::ICR_LO, static_cast<uint32_t>(v));
            }
        }

        /*
         * Write Local Vector Table Register
         *
         * @param r     LAPIC LVT register
         * @param d     Delivery mode
         * @param v     Vector
         * @param misc  Miscellaneous bits
         */
        static void set_lvt (Register32 r, Delivery d, unsigned v, unsigned misc = 0)
        {
            write (r, misc | std::to_underlying (d) | v);
        }

        /*
         * Lookup CPU Number
         *
         * @param i     LAPIC ID
         * @return      CPU number to which the LAPIC ID belongs
         */
        static auto lookup (apic_t i)
        {
            for (cpu_t c { 0 }; c < Cpu::count; c++)
                if (id[c] == i)
                    return c;

            return static_cast<cpu_t>(-1);
        }

        static inline unsigned ratio { 0 };

    public:
        static inline bool      x2apic      { false };
        static inline apic_t    id[NUM_CPU] { 0 };

        static auto time()      { return __builtin_ia32_rdtsc(); }
        static auto eoi_sup()   { return read (Register32::LVR) >> 24 & BIT (0); }
        static auto lvt_max()   { return read (Register32::LVR) >> 16 & BIT_RANGE (7, 0); }
        static auto version()   { return read (Register32::LVR)       & BIT_RANGE (7, 0); }
        static void eoi()       { write (Register32::EOI, 0); }

        /*
         * Send IPI to a specific CPU
         *
         * @param v     Vector
         * @param c     CPU number
         * @param d     Delivery mode
         */
        static void send_cpu (unsigned v, cpu_t c, Delivery d = Delivery::DLV_FIXED)
        {
            set_icr ((x2apic ? static_cast<uint64_t>(Cpu::remote_topology (c)) << 32 : static_cast<uint64_t>(id[c]) << 56) | BIT (14) | std::to_underlying (d) | v);
        }

        /*
         * Send IPI to all CPUs (excluding self)
         *
         * @param v     Vector
         * @param d     Delivery mode
         */
        static void send_exc (unsigned v, Delivery d = Delivery::DLV_FIXED)
        {
            set_icr (BIT_RANGE (19, 18) | BIT (14) | std::to_underlying (d) | v);
        }

        /*
         * Program Timer
         *
         * @param tsc   TSC deadline
         */
        static void set_timer (uint64_t tsc)
        {
            if (EXPECT_TRUE (!ratio))
                Msr::write (Msr::Register::IA32_TSC_DEADLINE, tsc);

            else {
                auto const now { time() };
                auto const icr { static_cast<uint32_t>((tsc - now) / ratio) };
                write (Register32::TMR_ICR, tsc > now && icr ? icr : 1);
            }
        }

        static void timer_handler();
        static void error_handler();
        static void perfm_handler();
        static void therm_handler();

        static void init (uint32_t, uint32_t);
};
