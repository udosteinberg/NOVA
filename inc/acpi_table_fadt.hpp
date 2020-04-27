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
#include "acpi_table.hpp"

#pragma pack(1)

/*
 * 5.2.9: Fixed ACPI Description Table (FADT)
 */
class Acpi_table_fadt : private Acpi_table
{
    private:
        uint32      facs32;                         //  36
        uint32      dsdt32;                         //  40
        uint8       int_model;                      //  44 (only ACPI 1.0)
        uint8       pm_profile;                     //  45
        uint16      sci_irq;                        //  46 (not for HW-reduced)
        uint32      smi_cmd;                        //  48 (not for HW-reduced)
        uint8       acpi_enable;                    //  52 (not for HW-reduced)
        uint8       acpi_disable;                   //  53 (not for HW-reduced)
        uint8       s4_bios_req;                    //  54 (not for HW-reduced)
        uint8       pstate_cnt;                     //  55 (not for HW-reduced)
        uint32      pm1a_evt_blk;                   //  56 (not for HW-reduced)
        uint32      pm1b_evt_blk;                   //  60 (not for HW-reduced)
        uint32      pm1a_cnt_blk;                   //  64 (not for HW-reduced)
        uint32      pm1b_cnt_blk;                   //  68 (not for HW-reduced)
        uint32      pm2_cnt_blk;                    //  72 (not for HW-reduced)
        uint32      pm_tmr_blk;                     //  76 (not for HW-reduced)
        uint32      gpe0_blk;                       //  80 (not for HW-reduced)
        uint32      gpe1_blk;                       //  84 (not for HW-reduced)
        uint8       pm1_evt_len;                    //  88 (not for HW-reduced)
        uint8       pm1_cnt_len;                    //  89 (not for HW-reduced)
        uint8       pm2_cnt_len;                    //  90 (not for HW-reduced)
        uint8       pm_tmr_len;                     //  91 (not for HW-reduced)
        uint8       gpe0_blk_len;                   //  92 (not for HW-reduced)
        uint8       gpe1_blk_len;                   //  93 (not for HW-reduced)
        uint8       gpe1_base;                      //  94 (not for HW-reduced)
        uint8       cstate_cnt;                     //  95 (not for HW-reduced)
        uint16      p_lvl2_lat;                     //  96 (not for HW-reduced)
        uint16      p_lvl3_lat;                     //  98 (not for HW-reduced)
        uint16      flush_size;                     // 100 (not for HW-reduced)
        uint16      flush_stride;                   // 102 (not for HW-reduced)
        uint8       duty_offset;                    // 104 (not for HW-reduced)
        uint8       duty_width;                     // 105 (not for HW-reduced)
        uint8       day_alarm;                      // 106 (not for HW-reduced)
        uint8       mon_alarm;                      // 107 (not for HW-reduced)
        uint8       century;                        // 108 (not for HW-reduced)
        uint16      bflg_x86;                       // 109
        uint8       reserved;                       // 111
        uint32      fflg;                           // 112
        Acpi_gas    reset_reg;                      // 116
        uint8       reset_value;                    // 128
        uint16      bflg_arm;                       // 129
        uint8       minor_version;                  // 131
        uint64      facs64;                         // 132
        uint64      dsdt64;                         // 140
        Acpi_gas    x_pm1a_evt_blk;                 // 148 (not for HW-reduced)
        Acpi_gas    x_pm1b_evt_blk;                 // 160 (not for HW-reduced)
        Acpi_gas    x_pm1a_cnt_blk;                 // 172 (not for HW-reduced)
        Acpi_gas    x_pm1b_cnt_blk;                 // 184 (not for HW-reduced)
        Acpi_gas    x_pm2_cnt_blk;                  // 196 (not for HW-reduced)
        Acpi_gas    x_pm_tmr_blk;                   // 208 (not for HW-reduced)
        Acpi_gas    x_gpe0_blk;                     // 220 (not for HW-reduced)
        Acpi_gas    x_gpe1_blk;                     // 232 (not for HW-reduced)
        Acpi_gas    sleep_control_reg;              // 244
        Acpi_gas    sleep_status_reg;               // 256
        uint64      hypervisor_vendor_id;           // 268

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
            WBINVD_FLUSH                = BIT  (1), // (not for HW-reduced)
            PROC_C1                     = BIT  (2), // (not for HW-reduced)
            P_LVL2_UP                   = BIT  (3), // (not for HW-reduced)
            PWR_BUTTON                  = BIT  (4),
            SLP_BUTTON                  = BIT  (5),
            FIX_RTC                     = BIT  (6),
            RTC_S4                      = BIT  (7), // (not for HW-reduced)
            TMR_VAL_EXT                 = BIT  (8), // (not for HW-reduced)
            DCK_CAP                     = BIT  (9),
            RESET_REG_SUP               = BIT (10),
            SEALED_CASE                 = BIT (11),
            HEADLESS                    = BIT (12),
            CPU_SW_SLP                  = BIT (13), // (not for HW-reduced)
            PCI_EXP_WAK                 = BIT (14), // (not for HW-reduced)
            USE_PLATFORM_CLOCK          = BIT (15),
            S4_RTC_STS_VALID            = BIT (16), // (not for HW-reduced)
            REMOTE_POWER_ON_CAPABLE     = BIT (17), // (not for HW-reduced)
            FORCE_APIC_CLUSTER_MODEL    = BIT (18),
            FORCE_APIC_PHYSICAL_MODE    = BIT (19),
            HW_REDUCED_ACPI             = BIT (20),
            LOW_POWER_S0_IDLE_CAPABLE   = BIT (21),
        };

    public:
        void parse() const;
};

#pragma pack()
