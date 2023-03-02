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

class Tpm final
{
    private:
        // TPM Interface
        enum class Interface : unsigned
        {
            FIFO_PTP            = 0x0,      // FIFO according to PTP 2.0
            CRB                 = 0x1,      // CRB
            FIFO_TIS            = 0xf,      // FIFO according to TIS 1.3
        };

        static inline Interface ifc;

        // TPM Family
        enum class Family : unsigned
        {
            TPM_12              = 0x0,      // TPM 1.2
            TPM_20              = 0x1,      // TPM 2.0
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

        // Relative timeout in ms converted to TSC ticks = freq_in_hz * ms / 1000
        static constexpr auto timeout (Timeout t) { return 1000'000'000ULL * std::to_underlying (t) / 1000; }

        enum class Register8 : unsigned
        {
            ACCESS              = 0x000,                // rw   TIS (banked per locality)
            INT_VECTOR          = 0x00c,                // rw   TIS
            RID                 = 0xf04,                // r-   TIS
        };

        enum class Register32 : unsigned
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

        /*
         * Locality 0: Legacy TPM Operation
         * Locality 1: Trusted Operating System
         * Locality 2: MLE
         * Locality 3: ACM
         * Locality 4: TXT HW
         */
        static auto read  (unsigned l, Register8  r)      { return *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TXTD | l << 12 | std::to_underlying (r)); }
        static auto read  (unsigned l, Register32 r)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TXTD | l << 12 | std::to_underlying (r)); }
        static void write (unsigned l, Register8  r, uint8_t  v) { *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TXTD | l << 12 | std::to_underlying (r)) = v; }
        static void write (unsigned l, Register32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TXTD | l << 12 | std::to_underlying (r)) = v; }

        [[nodiscard]] static bool poll_access (unsigned l, uint8_t m, uint8_t v)
        {
            for (auto const t { timeout (Timeout::A) }, b { __builtin_ia32_rdtsc() }; (read (l, Register8::ACCESS) & m) != v; __builtin_ia32_pause())
                if (EXPECT_FALSE (__builtin_ia32_rdtsc() - b > t))
                    return false;

            return true;
        }

        [[nodiscard]] static bool poll_status (unsigned l, uint32_t m, uint32_t v)
        {
            for (auto const t { timeout (Timeout::B) }, b { __builtin_ia32_rdtsc() }; (read (l, Register32::STATUS) & m) != v; __builtin_ia32_pause())
                if (EXPECT_FALSE (__builtin_ia32_rdtsc() - b > t))
                    return false;

            return true;
        }

        [[nodiscard]] static bool locality_request (unsigned l)
        {
            write (l, Register8::ACCESS, Access::REQUEST_USE);

            return poll_access (l, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY | Access::REQUEST_USE, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY);
        }

        [[nodiscard]] static bool locality_release (unsigned l)
        {
            write (l, Register8::ACCESS, Access::ACTIVE_LOCALITY);

            return poll_access (l, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY | Access::REQUEST_USE, Access::ACCESS_VALID);
        }

        [[nodiscard]] static bool make_ready (unsigned l)
        {
            write (l, Register32::STATUS, Status::COMMAND_READY);

            return poll_status (l, Status::COMMAND_READY, Status::COMMAND_READY);
        }

    public:
        [[nodiscard]] static bool init();

        static void info();
};
