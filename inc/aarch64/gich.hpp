/*
 * Generic Interrupt Controller: Hypervisor Interface (GICH/ICH)
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
#include "gicc.hpp"
#include "memory.hpp"
#include "sysreg.hpp"
#include "types.hpp"

class Gich
{
    friend class Acpi_table_madt;

    private:
        enum class Register32
        {
            HCR         = 0x0000,   // v2 -- rw Hypervisor Control Register
            VTR         = 0x0004,   // v2 -- ro VGIC Type Register
            VMCR        = 0x0008,   // v2 -- rw Virtual Machine Control Register
            MISR        = 0x0010,   // v2 -- ro Maintenance Interrupt Status Register
            EISR        = 0x0020,   // v2 -- ro End Of Interrupt Status Register
            ELRSR       = 0x0030,   // v2 -- ro Empty List Register Status Register
        };

        enum class Array32
        {
            APR         = 0x00f0,   // v2 -- rw Active Priorities Registers
            LR          = 0x0100,   // v2 -- rw List Registers
        };

        static inline uint64 phys   { Board::gic[3].mmio };

        static inline auto read  (Register32 r)             { return *reinterpret_cast<uint32 volatile *>(MMAP_GLB_GICH + static_cast<unsigned>(r)); }
        static inline auto read  (Array32 r, unsigned n)    { return *reinterpret_cast<uint32 volatile *>(MMAP_GLB_GICH + static_cast<unsigned>(r) + n * sizeof (uint32)); }

        static inline void write (Register32 r, uint32 v)          { *reinterpret_cast<uint32 volatile *>(MMAP_GLB_GICH + static_cast<unsigned>(r)) = v; }
        static inline void write (Array32 r, unsigned n, uint32 v) { *reinterpret_cast<uint32 volatile *>(MMAP_GLB_GICH + static_cast<unsigned>(r) + n * sizeof (uint32)) = v; }

        DEFINE_SYSREG32_RW (el2_ap0r0,      S3_4_C12_C8_0);
        DEFINE_SYSREG32_RW (el2_ap0r1,      S3_4_C12_C8_1);
        DEFINE_SYSREG32_RW (el2_ap0r2,      S3_4_C12_C8_2);
        DEFINE_SYSREG32_RW (el2_ap0r3,      S3_4_C12_C8_3);
        DEFINE_SYSREG32_RW (el2_ap1r0,      S3_4_C12_C9_0);
        DEFINE_SYSREG32_RW (el2_ap1r1,      S3_4_C12_C9_1);
        DEFINE_SYSREG32_RW (el2_ap1r2,      S3_4_C12_C9_2);
        DEFINE_SYSREG32_RW (el2_ap1r3,      S3_4_C12_C9_3);
        DEFINE_SYSREG32_RW (el2_hcr,        S3_4_C12_C11_0);
        DEFINE_SYSREG32_RO (el2_vtr,        S3_4_C12_C11_1);
        DEFINE_SYSREG32_RO (el2_misr,       S3_4_C12_C11_2);
        DEFINE_SYSREG32_RO (el2_eisr,       S3_4_C12_C11_3);
        DEFINE_SYSREG32_RO (el2_elrsr,      S3_4_C12_C11_5);
        DEFINE_SYSREG32_RW (el2_vmcr,       S3_4_C12_C11_7);
        DEFINE_SYSREG64_RW (el2_lr0,        S3_4_C12_C12_0);
        DEFINE_SYSREG64_RW (el2_lr1,        S3_4_C12_C12_1);
        DEFINE_SYSREG64_RW (el2_lr2,        S3_4_C12_C12_2);
        DEFINE_SYSREG64_RW (el2_lr3,        S3_4_C12_C12_3);
        DEFINE_SYSREG64_RW (el2_lr4,        S3_4_C12_C12_4);
        DEFINE_SYSREG64_RW (el2_lr5,        S3_4_C12_C12_5);
        DEFINE_SYSREG64_RW (el2_lr6,        S3_4_C12_C12_6);
        DEFINE_SYSREG64_RW (el2_lr7,        S3_4_C12_C12_7);
        DEFINE_SYSREG64_RW (el2_lr8,        S3_4_C12_C13_0);
        DEFINE_SYSREG64_RW (el2_lr9,        S3_4_C12_C13_1);
        DEFINE_SYSREG64_RW (el2_lr10,       S3_4_C12_C13_2);
        DEFINE_SYSREG64_RW (el2_lr11,       S3_4_C12_C13_3);
        DEFINE_SYSREG64_RW (el2_lr12,       S3_4_C12_C13_4);
        DEFINE_SYSREG64_RW (el2_lr13,       S3_4_C12_C13_5);
        DEFINE_SYSREG64_RW (el2_lr14,       S3_4_C12_C13_6);
        DEFINE_SYSREG64_RW (el2_lr15,       S3_4_C12_C13_7);

        static bool mmap_mmio();
        static void init_mmio();
        static void init_regs();

    public:
        static unsigned num_apr CPULOCAL;
        static unsigned num_lr  CPULOCAL;

        static void init();

        static void disable()
        {
            if (Gicc::mode == Gicc::Mode::REGS) {
                set_el2_hcr (0);
                Barrier::isb();
            } else
                write (Register32::HCR, 0);
        }

        static void load (uint64 const (&lr)[16], uint32 const (&ap0r)[4], uint32 const (&ap1r)[4], uint32 const &hcr, uint32 const &vmcr)
        {
            if (Gicc::mode == Gicc::Mode::REGS) {

                switch (num_lr) {
                    default:
                    case 16: set_el2_lr15 (lr[15]); FALLTHROUGH;
                    case 15: set_el2_lr14 (lr[14]); FALLTHROUGH;
                    case 14: set_el2_lr13 (lr[13]); FALLTHROUGH;
                    case 13: set_el2_lr12 (lr[12]); FALLTHROUGH;
                    case 12: set_el2_lr11 (lr[11]); FALLTHROUGH;
                    case 11: set_el2_lr10 (lr[10]); FALLTHROUGH;
                    case 10: set_el2_lr9  (lr[9]);  FALLTHROUGH;
                    case  9: set_el2_lr8  (lr[8]);  FALLTHROUGH;
                    case  8: set_el2_lr7  (lr[7]);  FALLTHROUGH;
                    case  7: set_el2_lr6  (lr[6]);  FALLTHROUGH;
                    case  6: set_el2_lr5  (lr[5]);  FALLTHROUGH;
                    case  5: set_el2_lr4  (lr[4]);  FALLTHROUGH;
                    case  4: set_el2_lr3  (lr[3]);  FALLTHROUGH;
                    case  3: set_el2_lr2  (lr[2]);  FALLTHROUGH;
                    case  2: set_el2_lr1  (lr[1]);  FALLTHROUGH;
                    case  1: set_el2_lr0  (lr[0]);  FALLTHROUGH;
                    case  0: break;
                }

                switch (num_apr) {
                    default:
                    case  4: set_el2_ap0r3 (ap0r[3]); set_el2_ap1r3 (ap1r[3]); FALLTHROUGH;
                    case  3: set_el2_ap0r2 (ap0r[2]); set_el2_ap1r2 (ap1r[2]); FALLTHROUGH;
                    case  2: set_el2_ap0r1 (ap0r[1]); set_el2_ap1r1 (ap1r[1]); FALLTHROUGH;
                    case  1: set_el2_ap0r0 (ap0r[0]); set_el2_ap1r0 (ap1r[0]); FALLTHROUGH;
                    case  0: break;
                }

                set_el2_vmcr (vmcr);
                set_el2_hcr  (hcr);

                Barrier::isb();

            } else {

                for (unsigned i = 0; i < num_lr; i++)
                    write (Array32::LR, i, static_cast<uint32>(lr[i]));

                for (unsigned i = 0; i < num_apr; i++)
                    write (Array32::APR, i, ap1r[i]);

                write (Register32::VMCR, vmcr);
                write (Register32::HCR,  hcr);
            }
        }

        static void save (uint64 (&lr)[16], uint32 (&ap0r)[4], uint32 (&ap1r)[4], uint32 &hcr, uint32 &vmcr, uint32 &elrsr)
        {
            if (Gicc::mode == Gicc::Mode::REGS) {

                switch (num_lr) {
                    default:
                    case 16: lr[15] = get_el2_lr15(); FALLTHROUGH;
                    case 15: lr[14] = get_el2_lr14(); FALLTHROUGH;
                    case 14: lr[13] = get_el2_lr13(); FALLTHROUGH;
                    case 13: lr[12] = get_el2_lr12(); FALLTHROUGH;
                    case 12: lr[11] = get_el2_lr11(); FALLTHROUGH;
                    case 11: lr[10] = get_el2_lr10(); FALLTHROUGH;
                    case 10: lr[9]  = get_el2_lr9();  FALLTHROUGH;
                    case  9: lr[8]  = get_el2_lr8();  FALLTHROUGH;
                    case  8: lr[7]  = get_el2_lr7();  FALLTHROUGH;
                    case  7: lr[6]  = get_el2_lr6();  FALLTHROUGH;
                    case  6: lr[5]  = get_el2_lr5();  FALLTHROUGH;
                    case  5: lr[4]  = get_el2_lr4();  FALLTHROUGH;
                    case  4: lr[3]  = get_el2_lr3();  FALLTHROUGH;
                    case  3: lr[2]  = get_el2_lr2();  FALLTHROUGH;
                    case  2: lr[1]  = get_el2_lr1();  FALLTHROUGH;
                    case  1: lr[0]  = get_el2_lr0();  FALLTHROUGH;
                    case  0: break;
                }

                switch (num_apr) {
                    default:
                    case  4: ap0r[3] = get_el2_ap0r3(); ap1r[3] = get_el2_ap1r3(); FALLTHROUGH;
                    case  3: ap0r[2] = get_el2_ap0r2(); ap1r[2] = get_el2_ap1r2(); FALLTHROUGH;
                    case  2: ap0r[1] = get_el2_ap0r1(); ap1r[1] = get_el2_ap1r1(); FALLTHROUGH;
                    case  1: ap0r[0] = get_el2_ap0r0(); ap1r[0] = get_el2_ap1r0(); FALLTHROUGH;
                    case  0: break;
                }

                elrsr = get_el2_elrsr();
                vmcr  = get_el2_vmcr();
                hcr   = get_el2_hcr();

            } else {

                for (unsigned i = 0; i < num_lr; i++)
                    lr[i] = read (Array32::LR, i);

                for (unsigned i = 0; i < num_apr; i++)
                    ap1r[i] = read (Array32::APR, i);

                elrsr = read (Register32::ELRSR);
                vmcr  = read (Register32::VMCR);
                hcr   = read (Register32::HCR);
            }
        }
};
