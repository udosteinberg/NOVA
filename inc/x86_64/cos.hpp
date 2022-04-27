/*
 * Class Of Service (COS)
 * Code/Data Prioritization (CDP)
 * Cache Allocation Technology (CAT)
 * Memory Bandwidth Allocation (MBA)
 *
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

#include "bits.hpp"
#include "macros.hpp"
#include "msr.hpp"
#include "status.hpp"

class Cos final
{
    friend class Cpu;

    private:
        enum Config
        {
            LOCKED  = BIT (0),
            CDP_L3  = BIT (1),
            CDP_L2  = BIT (2),
        };

        static uint8_t  config  CPULOCAL;       // Current QOS configuration
        static uint8_t  supcfg  CPULOCAL;       // Supported QOS configuration
        static uint8_t  cos_l3  CPULOCAL;       // Supported L3 COS MSRs
        static uint8_t  cos_l2  CPULOCAL;       // Supported L2 COS MSRs
        static uint8_t  cos_mb  CPULOCAL;       // Supported MB COS MSRs
        static uint8_t  hcb_l3  CPULOCAL;       // Supported L3 highest capacity bit
        static uint8_t  hcb_l2  CPULOCAL;       // Supported L2 highest capacity bit
        static uint16_t del_mb  CPULOCAL;       // Supported MB maximum delay value
        static cos_t    current CPULOCAL;       // Currently active COS

        static inline void set_cos (cos_t cos) { Msr::write (Msr::Register::IA32_PQR_ASSOC, static_cast<uint64_t>(cos) << 32); }

        static inline void set_l3_mask (uint16_t n, uint32_t val) { Msr::write (Msr::Array::IA32_L3_MASK, 1, n, val); }
        static inline void set_l2_mask (uint16_t n, uint32_t val) { Msr::write (Msr::Array::IA32_L2_MASK, 1, n, val); }
        static inline void set_mb_thrt (uint16_t n, uint16_t val) { Msr::write (Msr::Array::IA32_MB_THRT, 1, n, val); }

        static inline void set_qos_cfg (uint8_t cfg)
        {
            if (supcfg & Config::CDP_L3)    // IA32_L3_QOS_CFG exists
                Msr::write (Msr::Register::IA32_L3_QOS_CFG, !!(cfg & Config::CDP_L3) * BIT (0));

            if (supcfg & Config::CDP_L2)    // IA32_L2_QOS_CFG exists
                Msr::write (Msr::Register::IA32_L2_QOS_CFG, !!(cfg & Config::CDP_L2) * BIT (0));
        }

        /*
         * Determine if a capacity bitmask is valid
         *
         * All (and only) contiguous combinations of 1-bits up to the
         * highest capacity bit supported by hardware are allowed.
         *
         * @param cbm   Capacity bitmask
         * @param hcb   Highest capacity bit supported by hardware (must be < 32)
         * @return      True if valid, false otherwise
         */
        static inline bool valid_cbm (uint32_t cbm, uint8_t hcb)
        {
            if (!cbm || cbm > BIT_RANGE (hcb, 0))
                return false;

            auto const c { cbm >> bit_scan_forward (cbm) };

            return !(c & (c + 1));
        }

    public:
        /*
         * Determine if a class of service is valid
         *
         * In cases where features support a different number of COS, each
         * feature is active only for COS values within its supported range.
         *
         * @param cos   Class of service
         * @return      True if valid, false otherwise
         */
        static inline bool valid_cos (cos_t cos)
        {
            cos_t cos_num { 1 };

            // CDP must be configured, because it impacts the valid COS range
            if (config) {

                // If CDP is enabled, then the valid COS range is half the number of MSRs
                uint8_t const l3 = cos_l3 >> !!(config & Config::CDP_L3);
                if (cos_num < l3)
                    cos_num = l3;

                // If CDP is enabled, then the valid COS range is half the number of MSRs
                uint8_t const l2 = cos_l2 >> !!(config & Config::CDP_L2);
                if (cos_num < l2)
                    cos_num = l2;

                if (cos_num < cos_mb)
                    cos_num = cos_mb;
            }

            return cos < cos_num;
        }

        /*
         * Change the currently active class of service
         *
         * @param cos   Class of service to switch to
         */
        static inline void make_current (cos_t cos)
        {
            if (EXPECT_FALSE (current != cos))
                set_cos (current = cos);
        }

        /*
         * Configure QOS settings
         *
         * @param cfg   QOS configuration
         * @return      SUCCESS if QOS was configured, ABORTED if QOS had already been configured, BAD_PAR if invalid parameter
         */
        static inline auto cfg_qos (uint8_t cfg)
        {
            if (config)
                return Status::ABORTED;

            if ((supcfg & cfg) != cfg)
                return Status::BAD_PAR;

            set_qos_cfg (config = cfg | Config::LOCKED);

            return Status::SUCCESS;
        }

        /*
         * Configure CAT L3 parameters for a class of service
         *
         * @param n     L3 mask register number
         * @param cbm   Capacity bitmask
         * @return      SUCCESS if L3 cache mask was configured, ABORTED if QOS has not been configured, BAD_PAR if invalid parameter
         */
        static inline auto cfg_l3_mask (uint16_t n, uint32_t cbm)
        {
            if (!config)
                return Status::ABORTED;

            if (n >= cos_l3 || !valid_cbm (cbm, hcb_l3))
                return Status::BAD_PAR;

            set_l3_mask (n, cbm);

            return Status::SUCCESS;
        }

        /*
         * Configure CAT L2 parameters for a class of service
         *
         * @param n     L2 mask register number
         * @param cbm   Capacity bitmask
         * @return      SUCCESS if L2 cache mask was configured, ABORTED if QOS has not been configured, BAD_PAR if invalid parameter
         */
        static inline auto cfg_l2_mask (uint16_t n, uint32_t cbm)
        {
            if (!config)
                return Status::ABORTED;

            if (n >= cos_l2 || !valid_cbm (cbm, hcb_l2))
                return Status::BAD_PAR;

            set_l2_mask (n, cbm);

            return Status::SUCCESS;
        }

        /*
         * Configure MBA parameters for a class of service
         *
         * @param n     MBA throttling register number
         * @param del   MBA delay value
         * @return      SUCCESS if MBA delay value was configured, BAD_PAR otherwise
         */
        static inline auto cfg_mb_thrt (uint16_t n, uint16_t del)
        {
            if (!config)
                return Status::ABORTED;

            if (n >= cos_mb || del > del_mb)
                return Status::BAD_PAR;

            set_mb_thrt (n, del);

            return Status::SUCCESS;
        }

        static void init();
};
