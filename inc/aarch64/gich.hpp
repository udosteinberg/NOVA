/*
 * Generic Interrupt Controller: Hypervisor Interface (GICH/ICH)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

class Gich final
{
    friend class Acpi_table_madt;

    private:
        enum class Reg32 : unsigned
        {
            HCR         = 0x0000,   // v2 -- rw Hypervisor Control Register
            VTR         = 0x0004,   // v2 -- ro VGIC Type Register
            VMCR        = 0x0008,   // v2 -- rw Virtual Machine Control Register
            MISR        = 0x0010,   // v2 -- ro Maintenance Interrupt Status Register
            EISR        = 0x0020,   // v2 -- ro End Of Interrupt Status Register
            ELRSR       = 0x0030,   // v2 -- ro Empty List Register Status Register
        };

        enum class Arr32 : unsigned
        {
            APR         = 0x00f0,   // v2 -- rw Active Priorities Registers
            LR          = 0x0100,   // v2 -- rw List Registers
        };

        static inline uint64_t phys { Board::gic[3].mmio };

        static inline auto read  (Reg32 r)                  { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_GICH + std::to_underlying (r)); }
        static inline auto read  (Arr32 r, unsigned n)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_GICH + std::to_underlying (r) + n * sizeof (uint32_t)); }

        static inline void write (Reg32 r, uint32_t v)             { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_GICH + std::to_underlying (r)) = v; }
        static inline void write (Arr32 r, unsigned n, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_GICH + std::to_underlying (r) + n * sizeof (uint32_t)) = v; }

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

        static void mmap_mmio();
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
                write (Reg32::HCR, 0);
        }

        static void load (uint64_t const (&lr)[16], uint32_t const (&ap0r)[4], uint32_t const (&ap1r)[4], uint32_t const &hcr, uint32_t const &vmcr)
        {
            if (Gicc::mode == Gicc::Mode::REGS) {

                switch (num_lr) {
                    default:
                    case 16: set_el2_lr15 (lr[15]); [[fallthrough]];
                    case 15: set_el2_lr14 (lr[14]); [[fallthrough]];
                    case 14: set_el2_lr13 (lr[13]); [[fallthrough]];
                    case 13: set_el2_lr12 (lr[12]); [[fallthrough]];
                    case 12: set_el2_lr11 (lr[11]); [[fallthrough]];
                    case 11: set_el2_lr10 (lr[10]); [[fallthrough]];
                    case 10: set_el2_lr9  (lr[9]);  [[fallthrough]];
                    case  9: set_el2_lr8  (lr[8]);  [[fallthrough]];
                    case  8: set_el2_lr7  (lr[7]);  [[fallthrough]];
                    case  7: set_el2_lr6  (lr[6]);  [[fallthrough]];
                    case  6: set_el2_lr5  (lr[5]);  [[fallthrough]];
                    case  5: set_el2_lr4  (lr[4]);  [[fallthrough]];
                    case  4: set_el2_lr3  (lr[3]);  [[fallthrough]];
                    case  3: set_el2_lr2  (lr[2]);  [[fallthrough]];
                    case  2: set_el2_lr1  (lr[1]);  [[fallthrough]];
                    case  1: set_el2_lr0  (lr[0]);  [[fallthrough]];
                    case  0: break;
                }

                switch (num_apr) {
                    default:
                    case  4: set_el2_ap0r3 (ap0r[3]); set_el2_ap1r3 (ap1r[3]); [[fallthrough]];
                    case  3: set_el2_ap0r2 (ap0r[2]); set_el2_ap1r2 (ap1r[2]); [[fallthrough]];
                    case  2: set_el2_ap0r1 (ap0r[1]); set_el2_ap1r1 (ap1r[1]); [[fallthrough]];
                    case  1: set_el2_ap0r0 (ap0r[0]); set_el2_ap1r0 (ap1r[0]); [[fallthrough]];
                    case  0: break;
                }

                set_el2_vmcr (vmcr);
                set_el2_hcr  (hcr);

                Barrier::isb();

            } else {

                for (unsigned i { 0 }; i < num_lr; i++)
                    write (Arr32::LR, i, static_cast<uint32_t>(lr[i]));

                for (unsigned i { 0 }; i < num_apr; i++)
                    write (Arr32::APR, i, ap1r[i]);

                write (Reg32::VMCR, vmcr);
                write (Reg32::HCR,  hcr);
            }
        }

        static void save (uint64_t (&lr)[16], uint32_t (&ap0r)[4], uint32_t (&ap1r)[4], uint32_t &hcr, uint32_t &vmcr, uint32_t &elrsr)
        {
            if (Gicc::mode == Gicc::Mode::REGS) {

                switch (num_lr) {
                    default:
                    case 16: lr[15] = get_el2_lr15(); [[fallthrough]];
                    case 15: lr[14] = get_el2_lr14(); [[fallthrough]];
                    case 14: lr[13] = get_el2_lr13(); [[fallthrough]];
                    case 13: lr[12] = get_el2_lr12(); [[fallthrough]];
                    case 12: lr[11] = get_el2_lr11(); [[fallthrough]];
                    case 11: lr[10] = get_el2_lr10(); [[fallthrough]];
                    case 10: lr[9]  = get_el2_lr9();  [[fallthrough]];
                    case  9: lr[8]  = get_el2_lr8();  [[fallthrough]];
                    case  8: lr[7]  = get_el2_lr7();  [[fallthrough]];
                    case  7: lr[6]  = get_el2_lr6();  [[fallthrough]];
                    case  6: lr[5]  = get_el2_lr5();  [[fallthrough]];
                    case  5: lr[4]  = get_el2_lr4();  [[fallthrough]];
                    case  4: lr[3]  = get_el2_lr3();  [[fallthrough]];
                    case  3: lr[2]  = get_el2_lr2();  [[fallthrough]];
                    case  2: lr[1]  = get_el2_lr1();  [[fallthrough]];
                    case  1: lr[0]  = get_el2_lr0();  [[fallthrough]];
                    case  0: break;
                }

                switch (num_apr) {
                    default:
                    case  4: ap0r[3] = get_el2_ap0r3(); ap1r[3] = get_el2_ap1r3(); [[fallthrough]];
                    case  3: ap0r[2] = get_el2_ap0r2(); ap1r[2] = get_el2_ap1r2(); [[fallthrough]];
                    case  2: ap0r[1] = get_el2_ap0r1(); ap1r[1] = get_el2_ap1r1(); [[fallthrough]];
                    case  1: ap0r[0] = get_el2_ap0r0(); ap1r[0] = get_el2_ap1r0(); [[fallthrough]];
                    case  0: break;
                }

                elrsr = get_el2_elrsr();
                vmcr  = get_el2_vmcr();
                hcr   = get_el2_hcr();

            } else {

                for (unsigned i { 0 }; i < num_lr; i++)
                    lr[i] = read (Arr32::LR, i);

                for (unsigned i { 0 }; i < num_apr; i++)
                    ap1r[i] = read (Arr32::APR, i);

                elrsr = read (Reg32::ELRSR);
                vmcr  = read (Reg32::VMCR);
                hcr   = read (Reg32::HCR);
            }
        }
};
