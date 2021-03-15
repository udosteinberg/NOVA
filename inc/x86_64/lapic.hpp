/*
 * Local Advanced Programmable Interrupt Controller (LAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "cpu.hpp"
#include "lowlevel.hpp"
#include "memory.hpp"
#include "msr.hpp"

class Lapic
{
    private:
        enum class Register32
        {
            IDR         = 0x2,
            LVR         = 0x3,
            TPR         = 0x8,
            PPR         = 0xa,
            EOI         = 0xb,
            LDR         = 0xd,
            DFR         = 0xe,
            SVR         = 0xf,
            ISR         = 0x10,
            TMR         = 0x18,
            IRR         = 0x20,
            ESR         = 0x28,
            ICR         = 0x30,
            LVT_TIMER   = 0x32,
            LVT_THERM   = 0x33,
            LVT_PERFM   = 0x34,
            LVT_LINT0   = 0x35,
            LVT_LINT1   = 0x36,
            LVT_ERROR   = 0x37,
            TMR_ICR     = 0x38,
            TMR_CCR     = 0x39,
            TMR_DCR     = 0x3e,
            IPI_SELF    = 0x3f,
        };

        enum class Register64
        {
            ICR         = 0x30,
        };

        enum class Delivery
        {
            DLV_FIXED   = VAL_SHIFT (0, 8),
            DLV_NMI     = VAL_SHIFT (4, 8),
            DLV_INIT    = VAL_SHIFT (5, 8),
            DLV_SIPI    = VAL_SHIFT (6, 8),
            DLV_EXTINT  = VAL_SHIFT (7, 8),
        };

        ALWAYS_INLINE
        static inline auto read (Register32 r)
        {
            return *reinterpret_cast<uint32 volatile *>(MMAP_CPU_APIC + (static_cast<unsigned>(r) << 4));
        }

        ALWAYS_INLINE
        static inline void write (Register32 r, uint32 v)
        {
            *reinterpret_cast<uint32 volatile *>(MMAP_CPU_APIC + (static_cast<unsigned>(r) << 4)) = v;
        }

        ALWAYS_INLINE
        static inline void write (Register64 r, uint64 v)
        {
            write (Register32 (static_cast<unsigned>(r) + 1), static_cast<uint32>(v >> 32));
            write (Register32 (static_cast<unsigned>(r) + 0), static_cast<uint32>(v));
        }

        ALWAYS_INLINE
        static inline void set_lvt (Register32 r, Delivery d, unsigned v, unsigned misc = 0)
        {
            write (r, misc | static_cast<uint32>(d) | v);
        }

        ALWAYS_INLINE
        static inline void set_icr (uint64 v)
        {
            while (EXPECT_FALSE (read (Register32::ICR) & BIT (12)))
                pause();

            write (Register64::ICR, v);
        }

        static inline unsigned ratio { 0 };

    public:
        static inline auto time()       { return __builtin_ia32_rdtsc(); }
        static inline auto id()         { return read (Register32::IDR) >> 24 & BIT_RANGE (7, 0); }
        static inline auto eoi_sup()    { return read (Register32::LVR) >> 24 & BIT (0); }
        static inline auto lvt_max()    { return read (Register32::LVR) >> 16 & BIT_RANGE (7, 0); }
        static inline auto version()    { return read (Register32::LVR)       & BIT_RANGE (7, 0); }
        static inline auto get_timer()  { return read (Register32::TMR_CCR); }

        static inline void eoi()        { write (Register32::EOI, 0); }

        static inline void send_cpu (unsigned v, unsigned c, Delivery d = Delivery::DLV_FIXED)
        {
            set_icr (static_cast<uint64>(Cpu::apic_id[c]) << 56 | BIT (14) | static_cast<uint32>(d) | v);
        }

        static inline void send_exc (unsigned v, Delivery d = Delivery::DLV_FIXED)
        {
            set_icr (BIT_RANGE (19, 18) | BIT (14) | static_cast<uint32>(d) | v);
        }

        ALWAYS_INLINE
        static inline void set_timer (uint64 tsc)
        {
            if (ratio) {
                uint64 now = time();
                uint32 icr;
                write (Register32::TMR_ICR, tsc > now && (icr = static_cast<uint32>(tsc - now) / ratio) > 0 ? icr : 1);
            } else
                Msr::write (Msr::Register::IA32_TSC_DEADLINE, tsc);
        }

        static void timer_handler();
        static void error_handler();
        static void perfm_handler();
        static void therm_handler();

        static void init (uint32, uint32);
};
