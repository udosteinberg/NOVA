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
#include "tcg.hpp"
#include "wait.hpp"

class Tpm final : private Tcg
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

        enum class Locality : unsigned                  // Use  Reset       Extend
        {
            L0                  = 0x0000,               // Leg  16,23       0-16,23
            L1                  = 0x1000,               // TOS  16,23       0-16,20,23
            L2                  = 0x2000,               // MLE  16,20-23    0-23
            L3                  = 0x3000,               // ACM  16,20-23    0-20,23
            L4                  = 0x4000,               // TXT  16,20-23    0-18,23
        };

        enum class Reg8 : unsigned
        {
            ACCESS              = 0x000,                // rw   TIS (banked per locality)
            INT_VECTOR          = 0x00c,                // rw   TIS
            DATA_FIFO           = 0x024,                // rw   TIS
            RID                 = 0xf04,                // r-   TIS
        };

        enum class Reg32 : unsigned
        {
            INT_ENABLE          = 0x008,                // rw   TIS
            INT_STATUS          = 0x010,                // rw   TIS
            INTF_CAPABILITY     = 0x014,                // r-   TIS
            STATUS              = 0x018,                // rw   TIS
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

        [[nodiscard]] static auto burstcount (Locality l, unsigned &b)
        {
            return Wait::until (std::to_underlying (Timeout::A), [&] { return b = static_cast<uint16_t>(read (l, Reg32::STATUS) >> 8); });
        }

        [[nodiscard]] static auto poll_access (Locality l, uint8_t m, uint8_t v)
        {
            return Wait::until (std::to_underlying (Timeout::A), [&] { return (read (l, Reg8::ACCESS) & m) == v; });
        }

        [[nodiscard]] static auto poll_status (Locality l, uint32_t m, uint32_t v)
        {
            return Wait::until (std::to_underlying (Timeout::B), [&] { return (read (l, Reg32::STATUS) & m) == v; });
        }

        [[nodiscard]] static auto done_send (Locality l)
        {
            uint32_t s;
            return Wait::until (std::to_underlying (Timeout::A), [&] { return (s = read (l, Reg32::STATUS)) & Status::STATUS_VALID; }) && !(s & Status::EXPECT);
        }

        [[nodiscard]] static auto done_recv (Locality l)
        {
            uint32_t s;
            return Wait::until (std::to_underlying (Timeout::A), [&] { return (s = read (l, Reg32::STATUS)) & Status::STATUS_VALID; }) && !(s & Status::DATA_AVAIL);
        }

        [[nodiscard]] static auto make_ready (Locality l)
        {
            write (l, Reg32::STATUS, Status::COMMAND_READY);

            return poll_status (l, Status::COMMAND_READY, Status::COMMAND_READY);
        }

        [[nodiscard]] static auto request (Locality l)
        {
            write (l, Reg8::ACCESS, Access::REQUEST_USE);

            return poll_access (l, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY | Access::REQUEST_USE, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY);
        }

        [[nodiscard]] static auto release (Locality l)
        {
            write (l, Reg8::ACCESS, Access::ACTIVE_LOCALITY);

            return poll_access (l, Access::ACCESS_VALID | Access::ACTIVE_LOCALITY | Access::REQUEST_USE, Access::ACCESS_VALID);
        }

        class Cmd_guard final
        {
            private:
                Locality const loc;

            public:
                Cmd_guard (Locality l) : loc { l }
                {
                    write (loc, Reg32::STATUS, Status::COMMAND_READY);      // Prepare Command
                }

                ~Cmd_guard()
                {
                    write (loc, Reg32::STATUS, Status::COMMAND_READY);      // Ack/Cancel Command
                }

                Cmd_guard            (Cmd_guard const &) = delete;
                Cmd_guard& operator= (Cmd_guard const &) = delete;
        };

        [[nodiscard]] static bool exec (Locality, Cmd const &);
        [[nodiscard]] static bool send (Locality, Cmd const &);
        [[nodiscard]] static bool recv (Locality);

        [[nodiscard]] static bool v2_shutdown() { return exec (nova, Tpm20_shutdown { Tpm_su::Type::SU_CLEAR }); }
        [[nodiscard]] static bool v1_shutdown() { return exec (nova, Tpm12_shutdown {}); }

        [[nodiscard]] static bool v2_pcr_reset (unsigned pcr) { return exec (nova, Tpm20_pcr_reset { pcr }); }
        [[nodiscard]] static bool v1_pcr_reset (unsigned pcr) { return exec (nova, Tpm12_pcr_reset { pcr }); }

        template <typename T>
        [[nodiscard]] static bool v2_pcr_extend (unsigned pcr,             T &hash) { return exec (nova, Tpm20_pcr_extend { pcr, hash }); }
        [[nodiscard]] static bool v1_pcr_extend (unsigned pcr, Hash_sha1_160 &hash) { return exec (nova, Tpm12_pcr_extend { pcr, hash }); }

        static bool v2_pcr_read (unsigned);
        static bool v1_pcr_read (unsigned);

        static bool v2_cap_pcrs();
        static bool v2_cap_tpm_properties();
        static bool v1_cap_tpm_properties (uint32_t &, Tpm12_ptg::Type);

        static inline uint32_t tpm_mfr { 0 }, num_pcr { 0 }, max_buf { 0 }, max_dig { 0 };

        static inline uint8_t buffer[1024];

    public:
        static constexpr auto boot { Locality::L0 };
        static constexpr auto nova { Locality::L2 };

        static bool init (Locality);

        [[nodiscard]] static bool pcr_extend (unsigned pcr, Hash_sha1_160 &sha1_160, Hash_sha2_256 &sha2_256, Hash_sha2_384 &sha2_384, Hash_sha2_512 &sha2_512)
        {
            if (!request (nova))
                return false;

            bool complete { true };

            switch (fam) {

                case Family::TPM_12:
                    complete &= v1_pcr_extend (pcr, sha1_160);
                    break;

                case Family::TPM_20:
                    complete &= !supports (Hash_alg::SHA1_160) || v2_pcr_extend (pcr, sha1_160);
                    complete &= !supports (Hash_alg::SHA2_256) || v2_pcr_extend (pcr, sha2_256);
                    complete &= !supports (Hash_alg::SHA2_384) || v2_pcr_extend (pcr, sha2_384);
                    complete &= !supports (Hash_alg::SHA2_512) || v2_pcr_extend (pcr, sha2_512);
                    break;
            }

            for (unsigned i { 0 }; i < num_pcr; i++)
                fam == Family::TPM_20 ? v2_pcr_read (i) : v1_pcr_read (i);

            return release (nova) && complete;
        }
};
