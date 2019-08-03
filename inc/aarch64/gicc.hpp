/*
 * Generic Interrupt Controller: Physical CPU Interface (GICC/ICC)
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

#pragma once

#include "barrier.hpp"
#include "intid.hpp"
#include "memory.hpp"
#include "sysreg.hpp"
#include "types.hpp"

class Gicc : private Intid
{
    friend class Acpi_table_madt;

    private:
        enum class Register32
        {
            CTLR        = 0x0000,   // Control Register
            PMR         = 0x0004,   // Priority Mask Register
            BPR         = 0x0008,   // Binary Point Register
            IAR         = 0x000c,   // Interrupt Acknowledge Register
            EOIR        = 0x0010,   // End Of Interrupt Register
            RPR         = 0x0014,   // Running Priority Register
            HPPIR       = 0x0018,   // Highest Priority Pending Interrupt Register
            ABPR        = 0x001c,   // Aliased Binary Point Register
            AIAR        = 0x0020,   // Aliased Interrupt Acknowledge Register
            AEOIR       = 0x0024,   // Aliased End Of Interrupt Register
            AHPPIR      = 0x0028,   // Aliased Highest Priority Pending Interrupt Register
            APRn        = 0x00d0,   // Active Priorities Registers
            NSAPRn      = 0x00e0,   // Non-Secure Active Priorities Registers
            IIDR        = 0x00fc,   // Implementer Identification Register
            DIR         = 0x1000,   // Deactivate Interrupt Register
        };

        static inline uint64 phys   { Board::gic[2].mmio };

        static inline auto read  (Register32 r)    { return *reinterpret_cast<uint32 volatile *>(MMAP_GLB_GICC + static_cast<unsigned>(r)); }
        static inline void write (Register32 r, uint32 v) { *reinterpret_cast<uint32 volatile *>(MMAP_GLB_GICC + static_cast<unsigned>(r)) = v; }

        DEFINE_SYSREG_RW (el1_pmr,      S3_0_C4_C6_0);
        DEFINE_SYSREG_RO (el1_iar0,     S3_0_C12_C8_0);
        DEFINE_SYSREG_WO (el1_eoir0,    S3_0_C12_C8_1);
        DEFINE_SYSREG_RO (el1_hppir0,   S3_0_C12_C8_2);
        DEFINE_SYSREG_RW (el1_bpr0,     S3_0_C12_C8_3);
        DEFINE_SYSREG_WO (el1_dir,      S3_0_C12_C11_1);
        DEFINE_SYSREG_RO (el1_rpr,      S3_0_C12_C11_3);
        DEFINE_SYSREG_WO (el1_sgi1r,    S3_0_C12_C11_5);
        DEFINE_SYSREG_WO (el1_asgi1r,   S3_0_C12_C11_6);
        DEFINE_SYSREG_WO (el1_sgi0r,    S3_0_C12_C11_7);
        DEFINE_SYSREG_RO (el1_iar1,     S3_0_C12_C12_0);
        DEFINE_SYSREG_WO (el1_eoir1,    S3_0_C12_C12_1);
        DEFINE_SYSREG_RO (el1_hppir1,   S3_0_C12_C12_2);
        DEFINE_SYSREG_RW (el1_bpr1,     S3_0_C12_C12_3);
        DEFINE_SYSREG_RW (el1_ctlr,     S3_0_C12_C12_4);
        DEFINE_SYSREG_RW (el1_sre,      S3_0_C12_C12_5);
        DEFINE_SYSREG_RW (el1_igrpen0,  S3_0_C12_C12_6);
        DEFINE_SYSREG_RW (el1_igrpen1,  S3_0_C12_C12_7);
        DEFINE_SYSREG_RW (el2_sre,      S3_4_C12_C9_5);
        DEFINE_SYSREG_RW (el3_ctlr,     S3_6_C12_C12_4);
        DEFINE_SYSREG_RW (el3_sre,      S3_6_C12_C12_5);
        DEFINE_SYSREG_RW (el3_igrpen1,  S3_6_C12_C12_7);

        static void mmap_mmio();
        static void init_mmio();
        static void init_regs();
        static void init_mode();

        static inline void send_sgi (uint64 v)
        {
            // Ensure completion (global observability) of all previous stores
            Barrier::wmb_sync();

            set_el1_sgi1r (v);

            // Ensure system register write executed
            Barrier::isb();
        }

    public:
        enum class Mode
        {
            MMIO,
            REGS,
        };

        static inline Mode mode CPULOCAL { Mode::MMIO };

        static void init();

        static uint32 ack()
        {
            if (mode == Mode::REGS)
                return static_cast<uint32>(get_el1_iar1());
            else
                return read (Register32::IAR);
        }

        static void eoi (uint32 val)
        {
            if (mode == Mode::REGS) {
                set_el1_eoir1 (val);
                Barrier::isb();
            } else
                write (Register32::EOIR, val);
        }

        static void dir (uint32 val)
        {
            if (mode == Mode::REGS) {
                set_el1_dir (val);
                Barrier::isb();
            } else
                write (Register32::DIR, val);
        }

        static void send_cpu (unsigned, unsigned);
        static void send_exc (unsigned);
};
