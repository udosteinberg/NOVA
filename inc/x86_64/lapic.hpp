/*
 * Local Advanced Programmable Interrupt Controller (LAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "cpu.hpp"

class Lapic final
{
    private:
        enum class Reg32 : unsigned
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
            LVT_CMCHK   = 0x2f,             // Local Vector Table: Corrected Machine Check
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

        enum class Delivery : unsigned
        {
            DLV_FIXED   = 0,
            DLV_NMI     = 4,
            DLV_INIT    = 5,
            DLV_SIPI    = 6,
            DLV_EXTINT  = 7,
        };

        /*
         * Read 32-bit register using the x2APIC (MSR) or Legacy (MMIO) interface
         *
         * @param r     LAPIC register
         * @return      LAPIC register value
         */
        static auto read_legacy (Reg32 r) { return *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_APIC + (std::to_underlying (r) << 4)); }
        static auto read_x2apic (Reg32 r) { return static_cast<uint32_t>(Msr::read (Msr::Arr64::IA32_X2APIC, 1, std::to_underlying (r))); }

        static auto read (Reg32 r)
        {
            if (x2apic) [[likely]]
                return read_x2apic (r);
            else
                return read_legacy (r);
        }

        /*
         * Write 32-bit register using the x2APIC (MSR) or Legacy (MMIO) interface
         *
         * @param r     LAPIC register
         * @param v     LAPIC register value
         */
        static void write_legacy (Reg32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_CPU_APIC + (std::to_underlying (r) << 4)) = v; }
        static void write_x2apic (Reg32 r, uint32_t v) { Msr::write (Msr::Arr64::IA32_X2APIC, 1, std::to_underlying (r), v); }

        static void write (Reg32 r, uint32_t v)
        {
            if (x2apic) [[likely]]
                write_x2apic (r, v);
            else
                write_legacy (r, v);
        }

        /*
         * Write Interrupt Command Register
         *
         * @param v     LAPIC register value
         */
        static void set_icr (uint64_t v)
        {
            if (x2apic) [[likely]]
                Msr::write (Msr::Arr64::IA32_X2APIC, 1, std::to_underlying (Reg32::ICR_LO), v);

            else {
                while (read_legacy (Reg32::ICR_LO) & BIT (12)) [[unlikely]]
                    __builtin_ia32_pause();

                write_legacy (Reg32::ICR_HI, static_cast<uint32_t>(v >> 32));
                write_legacy (Reg32::ICR_LO, static_cast<uint32_t>(v));
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
        static void set_lvt (Reg32 r, Delivery d, unsigned v, unsigned misc = 0)
        {
            write (r, misc | std::to_underlying (d) << 8 | v);
        }

        /*
         * Lookup CPU Number
         *
         * @param i     LAPIC ID
         * @return      CPU number to which the LAPIC ID belongs
         */
        static cpu_t lookup (apic_t i)
        {
            for (cpu_t c { 0 }; c < Cpu::count; c++)
                if (id[c] == i)
                    return c;

            return 0xffff;
        }

        static inline constinit unsigned ratio { 0 };

    public:
        static constexpr auto msi_base { 0xfee00000 };
        static constexpr auto msi_size { 0x100000 };

        static inline constinit bool    x2apic      { false };
        static inline constinit apic_t  id[NUM_CPU] { 0 };

        static auto time()      { return static_cast<uint64_t>(__builtin_ia32_rdtsc()); }
        static auto eoi_sup()   { return read (Reg32::LVR) >> 24 & BIT (0); }
        static auto lvt_max()   { return read (Reg32::LVR) >> 16 & BIT_RANGE (7, 0); }
        static auto version()   { return read (Reg32::LVR)       & BIT_RANGE (7, 0); }
        static void eoi()       { write (Reg32::EOI, 0); }

        /*
         * Send IPI to a specific CPU
         *
         * @param v     Vector
         * @param c     CPU number
         * @param d     Delivery mode
         */
        static void send_cpu (unsigned v, cpu_t c, Delivery d = Delivery::DLV_FIXED)
        {
            set_icr ((x2apic ? static_cast<uint64_t>(Cpu::remote_topology (c)) << 32 : static_cast<uint64_t>(id[c]) << 56) | BIT (14) | std::to_underlying (d) << 8 | v);
        }

        /*
         * Send IPI to all CPUs (excluding self)
         *
         * @param v     Vector
         * @param d     Delivery mode
         */
        static void send_exc (unsigned v, Delivery d = Delivery::DLV_FIXED)
        {
            set_icr (BIT_RANGE (19, 18) | BIT (14) | std::to_underlying (d) << 8 | v);
        }

        /*
         * Program Timer
         *
         * @param tsc   TSC deadline
         */
        static void set_timer (uint64_t tsc)
        {
            if (!ratio) [[likely]]
                Msr::write (Msr::Reg64::IA32_TSC_DEADLINE, tsc);

            else {
                auto const now { time() };
                auto const icr { static_cast<uint32_t>((tsc - now) / ratio) };
                write (Reg32::TMR_ICR, tsc > now && icr ? icr : 1);
            }
        }

        static void handle_timer();
        static void handle_error();
        static void handle_perfm();
        static void handle_therm();
        static void handle_cmchk();

        static void init (uint32_t, uint32_t);
};
