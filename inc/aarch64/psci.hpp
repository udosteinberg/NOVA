/*
 * Power State Coordination Interface (PSCI)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

class Psci
{
    private:
        enum class Status
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

        enum class Function
        {
            PSCI_VERSION            = 0x0,   // 32
            CPU_SUSPEND             = 0x1,   // 64
            CPU_OFF                 = 0x2,   // 32
            CPU_ON                  = 0x3,   // 64
            AFFINITY_INFO           = 0x4,   // 64
            MIGRATE                 = 0x5,   // 64
            MIGRATE_INFO_TYPE       = 0x6,   // 32
            MIGRATE_INFO_UP_CPU     = 0x7,   // 64
            SYSTEM_OFF              = 0x8,   // 32
            SYSTEM_RESET            = 0x9,   // 32
            PSCI_FEATURES           = 0xa,   // 32
            CPU_FREEZE              = 0xb,   // 32
            CPU_DEFAULT_SUSPEND     = 0xc,   // 64
            NODE_HW_STATE           = 0xd,   // 64
            SYSTEM_SUSPEND          = 0xe,   // 64
            PSCI_SET_SUSPEND_MODE   = 0xf,   // 32
            PSCI_STAT_RESIDENCY     = 0x10,  // 64
            PSCI_STAT_COUNT         = 0x11,  // 64
            SYSTEM_RESET2           = 0x12,  // 64
            MEM_PROTECT             = 0x13,  // 32
            MEM_PROTECT_CHECK_RANGE = 0x14,  // 64
        };

        template <typename T>
        static mword call (Function f, mword &p1, mword &p2, mword &p3)
        {
            return Smc::call<T> (Smc::Service::SEC, Smc::Function (f), p1, p2, p3);
        }

        static uint32 version()
        {
            mword zero = 0;

            return uint32 (call<uint32> (Function::PSCI_VERSION, zero, zero, zero));
        }

        static Status cpu_on (mword cpu, mword addr, mword id)
        {
            return Status (call<uint64> (Function::CPU_ON, cpu, addr, id));
        }

        static Status system_reset()
        {
            mword zero = 0;

            return Status (call<uint32> (Function::SYSTEM_RESET, zero, zero, zero));
        }

    public:
        static void init();
        static void fini();
};
