/*
 * SMC ARCH Calls (Arm Architecture Calls)
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

class Smc_arch final : private Smc
{
    friend class Smc_psci;

    private:
        // ARCH Status Codes
        enum class Status : int
        {
            INVALID_PARAMETER       = -3,
            NOT_REQUIRED            = -2,
            NOT_SUPPORTED           = -1,
            SUCCESS                 =  0,
        };

        // ARCH SMC32 Functions
        enum class Function32 : uint16_t
        {
            VERSION                 = 0x0000,   // 1.1 Mandatory
            FEATURES                = 0x0001,   // 1.1 Mandatory
            SOC_ID                  = 0x0002,   // 1.0 Optional
            FEATURE_AVAILABILITY    = 0x0003,   // 1.1 Optional
            WORKAROUND_4            = 0x0004,   // 1.1 Optional
            WORKAROUND_3            = 0x3fff,   // 1.1 Optional
            WORKAROUND_2            = 0x7fff,   // 1.1 Optional
            WORKAROUND_1            = 0x8000,   // 1.1 Optional
        };

        // Convert ARCH function to SMC function identifier
        static constexpr auto function (Function32 f) { return Smc::function (Convention::SMC32, Service::ARM, std::to_underlying (f)); }

        // Invoke ARCH function
        [[nodiscard]] static auto call (Function32 f, uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0) { return Smc::call<Status>(function (f), p1, p2, p3); }

        /*
         * 7.2: Determine the implemented version of SMCCC
         */
        static auto version()
        {
            return static_cast<uint32_t>(call (Function32::VERSION));
        }

        /*
         * 7.3: Determine availability and features of the specified ARCH function
         */
        static auto features (fid_t id)
        {
            return call (Function32::FEATURES, id);
        }

    public:
        static inline constinit uint8_t workarounds { 0 };

        static void init();
};
