/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "acpi_gas.h"
#include "compiler.h"

class Acpi
{
    friend class Acpi_table_fadt;
    friend class Acpi_table_rsdt;
    friend class Acpi_rsdp;

    private:
        enum Register
        {
            PM1_STS,
            PM1_ENA,
            PM1_CNT,
            PM2_CNT,
            PM_TMR,
            RESET
        };

        enum PM1_Status
        {
            PM1_STS_TMR         = 1U << 0,      // 0x1
            PM1_STS_BM          = 1U << 4,      // 0x10
            PM1_STS_GBL         = 1U << 5,      // 0x20
            PM1_STS_PWRBTN      = 1U << 8,      // 0x100
            PM1_STS_SLPBTN      = 1U << 9,      // 0x200
            PM1_STS_RTC         = 1U << 10,     // 0x400
            PM1_STS_PCIE_WAKE   = 1U << 14,     // 0x4000
            PM1_STS_WAKE        = 1U << 15      // 0x8000
        };

        enum PM1_Enable
        {
            PM1_ENA_TMR         = 1U << 0,      // 0x1
            PM1_ENA_GBL         = 1U << 5,      // 0x20
            PM1_ENA_PWRBTN      = 1U << 8,      // 0x100
            PM1_ENA_SLPBTN      = 1U << 9,      // 0x200
            PM1_ENA_RTC         = 1U << 10,     // 0x400
            PM1_ENA_PCIE_WAKE   = 1U << 14      // 0x4000
        };

        enum PM1_Control
        {
            PM1_CNT_SCI_EN      = 1U << 0,      // 0x1
            PM1_CNT_BM_RLD      = 1U << 1,      // 0x2
            PM1_CNT_GBL_RLS     = 1U << 2,      // 0x4
            PM1_CNT_SLP_TYP     = 7U << 10,     // 0x400
            PM1_CNT_SLP_EN      = 1U << 13      // 0x2000
        };

        static unsigned const timer_frequency = 3579545;

        static Paddr dmar, fadt, madt, mcfg, rsdt, xsdt;

        static Acpi_gas pm1a_sts;
        static Acpi_gas pm1b_sts;
        static Acpi_gas pm1a_ena;
        static Acpi_gas pm1b_ena;
        static Acpi_gas pm1a_cnt;
        static Acpi_gas pm1b_cnt;
        static Acpi_gas pm2_cnt;
        static Acpi_gas pm_tmr;
        static Acpi_gas reset_reg;

        static uint32   tmr_ovf;
        static uint32   feature;
        static uint32   smi_cmd;
        static uint8    enable_val;
        static uint8    reset_val;

        static unsigned hw_read (Acpi_gas *);
        static unsigned read (Register);

        static void hw_write (Acpi_gas *, unsigned);
        static void write (Register, unsigned);

        ALWAYS_INLINE
        static inline mword tmr_msb() { return feature & 0x100 ? 31 : 23; }

        INIT
        static void enable();

        INIT
        static void setup_sci();

    public:
        static unsigned irq;
        static unsigned gsi;

        static void delay (unsigned);
        static uint64 time();
        static void reset();
        static void interrupt();

        INIT
        static void setup();
};
