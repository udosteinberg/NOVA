/*
 * SMC PSCI Calls (Power State Coordination Interface)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "smc.hpp"

class Smc_psci final : private Smc
{
    private:
        // PSCI Status Codes
        enum class Status : int
        {
            INVALID_ADDRESS         = -9,
            DISABLED                = -8,
            NOT_PRESENT             = -7,
            INTERNAL_FAILURE        = -6,
            ON_PENDING              = -5,
            ALREADY_ON              = -4,
            DENIED                  = -3,
            INVALID_PARAMETERS      = -2,
            NOT_SUPPORTED           = -1,
            SUCCESS                 =  0,
            AFFINITY_INFO_ON        =  0,
            AFFINITY_INFO_OFF       =  1,
            AFFINITY_INFO_ON_PEND   =  2,
        };

        // PSCI SMC32 Functions
        enum class Function32 : uint16_t
        {
            VERSION                 = 0x0000,   // 0.2 Mandatory
            CPU_OFF                 = 0x0002,   // 0.2 Mandatory
            MIGRATE_INFO_TYPE       = 0x0006,   // 0.2 Optional
            SYSTEM_OFF              = 0x0008,   // 0.2 Mandatory
            SYSTEM_RESET            = 0x0009,   // 0.2 Mandatory
            FEATURES                = 0x000a,   // 1.0 Mandatory
            CPU_FREEZE              = 0x000b,   // 1.0 Optional
            SET_SUSPEND_MODE        = 0x000f,   // 1.0 Optional
            MEM_PROTECT             = 0x0013,   // 1.1 Optional
        };

        // PSCI SMC64 Functions
        enum class Function64 : uint16_t
        {
            CPU_SUSPEND             = 0x0001,   // 0.2 Mandatory
            CPU_ON                  = 0x0003,   // 0.2 Mandatory
            AFFINITY_INFO           = 0x0004,   // 0.2 Mandatory
            MIGRATE                 = 0x0005,   // 0.2 Optional
            MIGRATE_INFO_UP_CPU     = 0x0007,   // 0.2 Optional
            CPU_DEFAULT_SUSPEND     = 0x000c,   // 1.0 Optional
            NODE_HW_STATE           = 0x000d,   // 1.0 Optional
            SYSTEM_SUSPEND          = 0x000e,   // 1.0 Optional
            STAT_RESIDENCY          = 0x0010,   // 1.0 Optional
            STAT_COUNT              = 0x0011,   // 1.0 Optional
            SYSTEM_RESET2           = 0x0012,   // 1.1 Optional
            MEM_PROTECT_CHECK_RANGE = 0x0014,   // 1.1 Optional
            SYSTEM_OFF2             = 0x0015,   // 1.3 Optional
        };

        // Convert PSCI function to SMC function identifier
        static constexpr auto function (Function32 f) { return Smc::function (Convention::SMC32, Service::SEC, std::to_underlying (f)); }
        static constexpr auto function (Function64 f) { return Smc::function (Convention::SMC64, Service::SEC, std::to_underlying (f)); }

        // Invoke PSCI function: Arguments are passed in R0...R3, return value in R0
        [[nodiscard]] static auto call (Function32 f, uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0) { return Smc::call<Status>(function (f), p1, p2, p3); }
        [[nodiscard]] static auto call (Function64 f, uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0) { return Smc::call<Status>(function (f), p1, p2, p3); }

        /*
         * 5.1.1: Determine the implemented version of PSCI
         */
        static auto version()
        {
            return static_cast<uint32_t>(call (Function32::VERSION));
        }

        /*
         * 5.1.15: Determine availability and features of the specified PSCI function (or SMCCC_VERSION)
         */
        static auto features (fid_t id)
        {
            return call (Function32::FEATURES, id);
        }

        /*
         * 5.1.4: Power up a core
         */
        static auto cpu_on (uint64_t cpu_affinity, uint64_t entry_addr, uint64_t context_id)
        {
            return call (Function64::CPU_ON, cpu_affinity, entry_addr, context_id);
        }

    public:
        static inline constinit uint8_t states { 0 };

        static void init();
        static bool boot_cpu (cpu_t, uint64_t);
        static void offline_wait();

        /*
         * 5.1.3: Power down the calling core
         */
        static auto cpu_off()
        {
            return call (Function32::CPU_OFF);
        }

        /*
         * 5.1.5: Request the status of an affinity instance
         */
        static auto affinity_info (uint64_t cpu_affinity)
        {
            return call (Function64::AFFINITY_INFO, cpu_affinity, 0);
        }

        /*
         * 5.1.9: Shut down the system
         */
        static auto system_off()
        {
            return call (Function32::SYSTEM_OFF);
        }

        /*
         * 5.1.11: Reset the system
         */
        static auto system_reset()
        {
            return call (Function32::SYSTEM_RESET);
        }

        /*
         * 5.1.19: Suspend the system
         */
        static auto system_suspend (uint64_t entry_addr, uint64_t context_id)
        {
            return call (Function64::SYSTEM_SUSPEND, entry_addr, context_id);
        }
};
