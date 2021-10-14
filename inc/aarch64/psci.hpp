/*
 * Power State Coordination Interface (PSCI)
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

#include "smc.hpp"

class Psci final : private Smc
{
    private:
        // Status Codes
        enum class Status : int
        {
            SUCCESS                 =  0,
            NOT_SUPPORTED           = -1,
            INVALID_PARAMETERS      = -2,
            DENIED                  = -3,
            ALREADY_ON              = -4,
            ON_PENDING              = -5,
            INTERNAL_FAILURE        = -6,
            NOT_PRESENT             = -7,
            DISABLED                = -8,
            INVALID_ADDRESS         = -9,
        };

        // SMC32 Functions
        enum class Function32 : uint16_t
        {
            PSCI_VERSION            = 0x0,   // 0.2 Mandatory
            CPU_OFF                 = 0x2,   // 0.2 Mandatory
            MIGRATE_INFO_TYPE       = 0x6,   // 0.2 Optional
            SYSTEM_OFF              = 0x8,   // 0.2 Mandatory
            SYSTEM_RESET            = 0x9,   // 0.2 Mandatory
            PSCI_FEATURES           = 0xa,   // 1.0 Mandatory
            CPU_FREEZE              = 0xb,   // 1.0 Optional
            PSCI_SET_SUSPEND_MODE   = 0xf,   // 1.0 Optional
            MEM_PROTECT             = 0x13,  // 1.1 Optional
        };

        // SMC64 Functions
        enum class Function64 : uint16_t
        {
            CPU_SUSPEND             = 0x1,   // 0.2 Mandatory
            CPU_ON                  = 0x3,   // 0.2 Mandatory
            AFFINITY_INFO           = 0x4,   // 0.2 Mandatory
            MIGRATE                 = 0x5,   // 0.2 Optional
            MIGRATE_INFO_UP_CPU     = 0x7,   // 0.2 Optional
            CPU_DEFAULT_SUSPEND     = 0xc,   // 1.0 Optional
            NODE_HW_STATE           = 0xd,   // 1.0 Optional
            SYSTEM_SUSPEND          = 0xe,   // 1.0 Optional
            PSCI_STAT_RESIDENCY     = 0x10,  // 1.0 Optional
            PSCI_STAT_COUNT         = 0x11,  // 1.0 Optional
            SYSTEM_RESET2           = 0x12,  // 1.1 Optional
            MEM_PROTECT_CHECK_RANGE = 0x14,  // 1.1 Optional
        };

        ALWAYS_INLINE
        static inline auto invoke (Function32 f, uintptr_t &p1, uintptr_t &p2, uintptr_t &p3)
        {
            return call (Call::SMC32, Service::SEC, std::to_underlying (f), p1, p2, p3);
        }

        ALWAYS_INLINE
        static inline auto invoke (Function64 f, uintptr_t &p1, uintptr_t &p2, uintptr_t &p3)
        {
            return call (Call::SMC64, Service::SEC, std::to_underlying (f), p1, p2, p3);
        }

        ALWAYS_INLINE
        static inline bool supported (Function32 f)
        {
            return features (fid (Call::SMC32, Service::SEC, std::to_underlying (f)));
        }

        ALWAYS_INLINE
        static inline bool supported (Function64 f)
        {
            return features (fid (Call::SMC64, Service::SEC, std::to_underlying (f)));
        }

        ALWAYS_INLINE
        static inline bool features (uintptr_t fid)
        {
            uintptr_t zero { 0 };

            return static_cast<Status>(invoke (Function32::PSCI_FEATURES, fid, zero, zero)) != Status::NOT_SUPPORTED;
        }

        ALWAYS_INLINE
        static inline auto version()
        {
            uintptr_t zero { 0 };

            return static_cast<uint32_t>(invoke (Function32::PSCI_VERSION, zero, zero, zero));
        }

        ALWAYS_INLINE
        static inline auto cpu_on (uintptr_t aff, uintptr_t addr, uintptr_t id)
        {
            return static_cast<Status>(invoke (Function64::CPU_ON, aff, addr, id));
        }

    public:
        static inline uint8_t states { 0 };

        static void init();

        static bool boot_cpu (cpu_t, uint64_t);

        static void offline_wait();

        /*
         * Shut down the calling core
         */
        ALWAYS_INLINE
        static inline bool cpu_off()
        {
            uintptr_t zero { 0 };

            invoke (Function32::CPU_OFF, zero, zero, zero);

            return false;   // Failed
        }

        /*
         * Query CPU offline state
         */
        ALWAYS_INLINE
        static inline bool is_cpu_off (uintptr_t aff)
        {
            uintptr_t zero { 0 };

            return invoke (Function64::AFFINITY_INFO, aff, zero, zero) == 1;
        }

        /*
         * Shut down the system
         */
        ALWAYS_INLINE
        static inline bool system_off()
        {
            uintptr_t zero { 0 };

            invoke (Function32::SYSTEM_OFF, zero, zero, zero);

            return false;   // Failed: Supposedly unreachable, but better be safe
        }

        /*
         * Reset the system
         */
        ALWAYS_INLINE
        static inline bool system_reset()
        {
            uintptr_t zero { 0 };

            invoke (Function32::SYSTEM_RESET, zero, zero, zero);

            return false;   // Failed: Supposedly unreachable, but better be safe
        }

        /*
         * Suspend the system
         */
        ALWAYS_INLINE
        static inline auto system_suspend (uintptr_t addr, uintptr_t id)
        {
            uintptr_t zero { 0 };

            invoke (Function64::SYSTEM_SUSPEND, addr, id, zero);

            return false;   // Failed
        }
};
