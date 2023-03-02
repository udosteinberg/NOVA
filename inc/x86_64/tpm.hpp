/*
 * Trusted Platform Module (TPM)
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

#include "memory.hpp"
#include "std.hpp"
#include "types.hpp"
#include "wait.hpp"

class Tpm final
{
    private:
        // TPM Interface
        enum class Interface : unsigned
        {
            FIFO_PTP            = 0x0,                  // FIFO according to PTP 2.0
            CRB                 = 0x1,                  // CRB
            FIFO_TIS            = 0xf,                  // FIFO according to TIS 1.3
        };

        static inline Interface ifc;

        // TPM Family
        enum class Family : unsigned
        {
            TPM_12              = 0x0,                  // TPM 1.2
            TPM_20              = 0x1,                  // TPM 2.0
        };

        static inline Family fam;

        // Default timeouts in ms
        enum class Timeout : unsigned
        {
            A                   =  750,
            B                   = 2000,
            C                   =  750,
            D                   =  750,
        };

        enum class Locality : unsigned
        {
            L0                  = 0x0000,               // Legacy TPM Operation
            L1                  = 0x1000,               // Trusted Operating System
            L2                  = 0x2000,               // MLE
            L3                  = 0x3000,               // ACM
            L4                  = 0x4000,               // TXT HW
        };

        enum class Reg8 : unsigned
        {
            ACCESS              = 0x000,                // rw   TIS (banked per locality)
            INT_VECTOR          = 0x00c,                // rw   TIS
            RID                 = 0xf04,                // r-   TIS
        };

        enum class Reg32 : unsigned
        {
            INT_ENABLE          = 0x008,                // rw   TIS
            INT_STATUS          = 0x010,                // rw   TIS
            INTF_CAPABILITY     = 0x014,                // r-   TIS
            STATUS              = 0x018,                // rw   TIS
            DATA_FIFO           = 0x024,                // rw   TIS
            INTERFACE_ID        = 0x030,                // rw   PTP (TIS returns 0xffffffff)
            XDATA_FIFO          = 0x080,                // rw   TIS
            DIDVID              = 0xf00,                // r-   TIS
        };

        enum Access : uint8_t                           // for write-only and res0 fields, reads return 0
        {
            ACCESS_VALID        = BIT (7),              // r-   TIS (always valid)
            ACCESS_RES0         = BIT (6),              // r-   TIS (only valid when ACCESS_VALID is 1)
            ACTIVE_LOCALITY     = BIT (5),              // rw   TIS (only valid when ACCESS_VALID is 1)
            BEEN_SEIZED         = BIT (4),              // rw   TIS (only valid when ACCESS_VALID is 1)
            SEIZE               = BIT (3),              // -w   TIS (only valid when ACCESS_VALID is 1)
            PENDING_REQUEST     = BIT (2),              // r-   TIS (only valid when ACCESS_VALID is 1)
            REQUEST_USE         = BIT (1),              // rw   TIS (only valid when ACCESS_VALID is 1)
            ESTABLISHMENT       = BIT (0),              // r-   TIS (only valid when ACCESS_VALID is 1)
        };

        enum Status : uint32_t                          // for write-only and res0 fields, reads return 0
        {
            TPM_FAMILY          = BIT_RANGE (27, 26),   // r-   PTP (always valid)
            RESET_ESTABLISHMENT = BIT (25),             // -w   PTP (always valid, locality 3/4 only)
            COMMAND_CANCEL      = BIT (24),             // -w   PTP (always valid)
            BURST_COUNT         = BIT_RANGE (23, 8),    // r-   TIS (always valid)
            STATUS_VALID        = BIT (7),              // r-   TIS (always valid)
            COMMAND_READY       = BIT (6),              // rw   TIS (always valid)
            TPM_GO              = BIT (5),              // -w   TIS (always valid)
            DATA_AVAIL          = BIT (4),              // r-   TIS (only valid when STATUS_VALID is 1)
            EXPECT              = BIT (3),              // r-   TIS (only valid when STATUS_VALID is 1)
            SELFTEST_DONE       = BIT (2),              // r-   TIS (always valid)
            RESPONSE_RETRY      = BIT (1),              // -w   TIS (always valid)
            STATUS_RES0         = BIT (0),              // r-   TIS (always valid)
        };

        static auto read  (Locality l, Reg8  r)      { return *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)); }
        static auto read  (Locality l, Reg32 r)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)); }
        static void write (Locality l, Reg8  r, uint8_t  v) { *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)) = v; }
        static void write (Locality l, Reg32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)) = v; }

        [[nodiscard]] static bool poll_access (Locality l, uint8_t m, uint8_t v)
        {
            return Wait::until (std::to_underlying (Timeout::A), [&] { return (read (l, Reg8::ACCESS) & m) == v; });
        }

        [[nodiscard]] static bool poll_status (Locality l, uint32_t m, uint32_t v)
        {
            return Wait::until (std::to_underlying (Timeout::B), [&] { return (read (l, Reg32::STATUS) & m) == v; });
        }

        [[nodiscard]] static bool locality_request (Locality l)
        {
            write (l, Reg8::ACCESS, Access::REQUEST_USE);

            return poll_access (l, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY | Access::REQUEST_USE, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY);
        }

        [[nodiscard]] static bool locality_release (Locality l)
        {
            write (l, Reg8::ACCESS, Access::ACTIVE_LOCALITY);

            return poll_access (l, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY | Access::REQUEST_USE, Access::ACCESS_VALID);
        }

        [[nodiscard]] static bool make_ready (Locality l)
        {
            write (l, Reg32::STATUS, Status::COMMAND_READY);

            return poll_status (l, Status::COMMAND_READY, Status::COMMAND_READY);
        }

    public:
        [[nodiscard]] static bool init();

        static void info();
};
