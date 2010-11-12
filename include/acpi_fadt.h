/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#include "acpi_table.h"

#pragma pack(1)

/*
 * Fixed ACPI Description Table (5.2.9)
 */
class Acpi_table_fadt : public Acpi_table
{
    public:
        uint32      firmware_ctrl;                  // 36
        uint32      dsdt_addr;                      // 40
        uint8       int_model;                      // 44
        uint8       pm_profile;
        uint16      sci_irq;
        uint32      smi_cmd;
        uint8       acpi_enable;
        uint8       acpi_disable;
        uint8       s4_bios_req;
        uint8       pstate_cnt;
        uint32      pm1a_evt_blk;
        uint32      pm1b_evt_blk;
        uint32      pm1a_cnt_blk;
        uint32      pm1b_cnt_blk;
        uint32      pm2_cnt_blk;
        uint32      pm_tmr_blk;
        uint32      gpe0_blk;
        uint32      gpe1_blk;
        uint8       pm1_evt_len;
        uint8       pm1_cnt_len;
        uint8       pm2_cnt_len;
        uint8       pm_tmr_len;
        uint8       gpe0_blk_len;
        uint8       gpe1_blk_len;
        uint8       gpe1_base;
        uint8       cstate_cnt;
        uint16      p_lvl2_lat;
        uint16      p_lvl3_lat;
        uint16      flush_size;
        uint16      flush_stride;
        uint8       duty_offset;
        uint8       duty_width;
        uint8       day_alarm;
        uint8       mon_alarm;
        uint8       century;                        // 108
        uint16      iapc_boot_arch;                 // 109
        uint8       reserved_1;                     // 111
        uint32      flags;                          // 112
        Acpi_gas    reset_reg;                      // 116
        uint8       reset_value;                    // 128
        uint8       reserved_2[3];                  // 129
        uint64      x_firmware_ctrl;                // 132
        uint64      x_dsdt_addr;                    // 140
        Acpi_gas    x_pm1a_evt_blk;                 // 148
        Acpi_gas    x_pm1b_evt_blk;                 // 160
        Acpi_gas    x_pm1a_cnt_blk;                 // 172
        Acpi_gas    x_pm1b_cnt_blk;                 // 184
        Acpi_gas    x_pm2_cnt_blk;                  // 196
        Acpi_gas    x_pm_tmr_blk;                   // 208
        Acpi_gas    x_gpe0_blk;                     // 220
        Acpi_gas    x_gpe1_blk;                     // 232

        enum Boot
        {
            HAS_LEGACY_DEVICES          = 1u << 0,
            HAS_8042                    = 1u << 1,
            NO_VGA                      = 1u << 2,
            NO_MSI                      = 1u << 3,
            NO_ASPM                     = 1u << 4
        };

        enum Feature
        {
            WBINVD                      = 1u << 0,
            WBINVD_FLUSH                = 1u << 1,
            PROC_C1                     = 1u << 2,
            P_LVL2_UP                   = 1u << 3,
            PWR_BUTTON                  = 1u << 4,
            SLP_BUTTON                  = 1u << 5,
            FIX_RTC                     = 1u << 6,
            RTC_S4                      = 1u << 7,
            TMR_VAL_EXT                 = 1u << 8,
            DCK_CAP                     = 1u << 9,
            RESET_REG_SUP               = 1u << 10,
            SEALED_CASE                 = 1u << 11,
            HEADLESS                    = 1u << 12,
            CPU_SW_SLP                  = 1u << 13,
            PCI_EXP_WAK                 = 1u << 14,
            USE_PLATFORM_CLOCK          = 1u << 15,
            S4_RTC_STS_VALID            = 1u << 16,
            REMOTE_POWER_ON_CAPABLE     = 1u << 17,
            FORCE_APIC_CLUSTER_MODEL    = 1u << 18,
            FORCE_APIC_PHYSICAL_MODE    = 1u << 19
        };

        INIT
        void parse() const;
};

#pragma pack()
