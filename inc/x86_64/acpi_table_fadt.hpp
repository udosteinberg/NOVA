/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "acpi_gas.hpp"
#include "acpi_table.hpp"

#pragma pack(1)

/*
 * 5.2.9: Fixed ACPI Description Table (FADT)
 */
class Acpi_table_fadt final
{
    private:
        Acpi_table  table;                          //   0 (1.0)
        uint32_t    facs32;                         //  36 (1.0)
        uint32_t    dsdt32;                         //  40 (1.0)
        uint8_t     int_model;                      //  44 (1.0 only)
        uint8_t     pm_profile;                     //  45 (2.0)
        uint16_t    sci_irq;                        //  46 (1.0) not for HW-reduced
        uint32_t    smi_cmd;                        //  48 (1.0) not for HW-reduced
        uint8_t     acpi_enable;                    //  52 (1.0) not for HW-reduced
        uint8_t     acpi_disable;                   //  53 (1.0) not for HW-reduced
        uint8_t     s4_bios_req;                    //  54 (1.0) not for HW-reduced
        uint8_t     pstate_cnt;                     //  55 (2.0) not for HW-reduced
        uint32_t    pm1a_evt_blk;                   //  56 (1.0) not for HW-reduced
        uint32_t    pm1b_evt_blk;                   //  60 (1.0) not for HW-reduced
        uint32_t    pm1a_cnt_blk;                   //  64 (1.0) not for HW-reduced
        uint32_t    pm1b_cnt_blk;                   //  68 (1.0) not for HW-reduced
        uint32_t    pm2_cnt_blk;                    //  72 (1.0) not for HW-reduced
        uint32_t    pm_tmr_blk;                     //  76 (1.0) not for HW-reduced
        uint32_t    gpe0_blk;                       //  80 (1.0) not for HW-reduced
        uint32_t    gpe1_blk;                       //  84 (1.0) not for HW-reduced
        uint8_t     pm1_evt_len;                    //  88 (1.0) not for HW-reduced
        uint8_t     pm1_cnt_len;                    //  89 (1.0) not for HW-reduced
        uint8_t     pm2_cnt_len;                    //  90 (1.0) not for HW-reduced
        uint8_t     pm_tmr_len;                     //  91 (1.0) not for HW-reduced
        uint8_t     gpe0_blk_len;                   //  92 (1.0) not for HW-reduced
        uint8_t     gpe1_blk_len;                   //  93 (1.0) not for HW-reduced
        uint8_t     gpe1_base;                      //  94 (1.0) not for HW-reduced
        uint8_t     cstate_cnt;                     //  95 (2.0) not for HW-reduced
        uint16_t    p_lvl2_lat;                     //  96 (1.0) not for HW-reduced
        uint16_t    p_lvl3_lat;                     //  98 (1.0) not for HW-reduced
        uint16_t    flush_size;                     // 100 (1.0) not for HW-reduced
        uint16_t    flush_stride;                   // 102 (1.0) not for HW-reduced
        uint8_t     duty_offset;                    // 104 (1.0) not for HW-reduced
        uint8_t     duty_width;                     // 105 (1.0) not for HW-reduced
        uint8_t     day_alarm;                      // 106 (1.0) not for HW-reduced
        uint8_t     mon_alarm;                      // 107 (1.0) not for HW-reduced
        uint8_t     century;                        // 108 (1.0) not for HW-reduced
        uint16_t    bflg_x86;                       // 109 (2.0)
        uint8_t     reserved;                       // 111
        uint32_t    fflg;                           // 112 (1.0)
        Acpi_gas    reset_reg;                      // 116 (2.0)
        uint8_t     reset_value;                    // 128 (2.0)
        uint16_t    bflg_arm;                       // 129 (5.1)
        uint8_t     minor_version;                  // 131 (5.1)
        uint64_t    facs64;                         // 132 (2.0)
        uint64_t    dsdt64;                         // 140 (2.0)
        Acpi_gas    x_pm1a_evt_blk;                 // 148 (2.0) not for HW-reduced
        Acpi_gas    x_pm1b_evt_blk;                 // 160 (2.0) not for HW-reduced
        Acpi_gas    x_pm1a_cnt_blk;                 // 172 (2.0) not for HW-reduced
        Acpi_gas    x_pm1b_cnt_blk;                 // 184 (2.0) not for HW-reduced
        Acpi_gas    x_pm2_cnt_blk;                  // 196 (2.0) not for HW-reduced
        Acpi_gas    x_pm_tmr_blk;                   // 208 (2.0) not for HW-reduced
        Acpi_gas    x_gpe0_blk;                     // 220 (2.0) not for HW-reduced
        Acpi_gas    x_gpe1_blk;                     // 232 (2.0) not for HW-reduced
        Acpi_gas    sleep_control_reg;              // 244 (5.0)
        Acpi_gas    sleep_status_reg;               // 256 (5.0)
        uint64_t    hypervisor_vendor_id;           // 268 (6.0)

        enum Boot_x86
        {
            HAS_LEGACY_DEVICES          = BIT  (0),
            HAS_8042                    = BIT  (1),
            NO_VGA                      = BIT  (2),
            NO_MSI                      = BIT  (3),
            NO_ASPM                     = BIT  (4),
            NO_CMOS_RTC                 = BIT  (5),
        };

        enum Boot_arm
        {
            PSCI_COMPLIANT              = BIT  (0),
            PSCI_USE_HVC                = BIT  (1),
        };

        enum Flags
        {
            WBINVD                      = BIT  (0),
            WBINVD_FLUSH                = BIT  (1), // not for HW-reduced
            PROC_C1                     = BIT  (2), // not for HW-reduced
            P_LVL2_UP                   = BIT  (3), // not for HW-reduced
            PWR_BUTTON                  = BIT  (4),
            SLP_BUTTON                  = BIT  (5),
            FIX_RTC                     = BIT  (6),
            RTC_S4                      = BIT  (7), // not for HW-reduced
            TMR_VAL_EXT                 = BIT  (8), // not for HW-reduced
            DCK_CAP                     = BIT  (9),
            RESET_REG_SUP               = BIT (10),
            SEALED_CASE                 = BIT (11),
            HEADLESS                    = BIT (12),
            CPU_SW_SLP                  = BIT (13), // not for HW-reduced
            PCI_EXP_WAK                 = BIT (14), // not for HW-reduced
            USE_PLATFORM_CLOCK          = BIT (15),
            S4_RTC_STS_VALID            = BIT (16), // not for HW-reduced
            REMOTE_POWER_ON_CAPABLE     = BIT (17), // not for HW-reduced
            FORCE_APIC_CLUSTER_MODEL    = BIT (18),
            FORCE_APIC_PHYSICAL_MODE    = BIT (19),
            HW_REDUCED_ACPI             = BIT (20),
            LOW_POWER_S0_IDLE_CAPABLE   = BIT (21),
        };

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_fadt) && sizeof (Acpi_table_fadt) == 276);

#pragma pack()
