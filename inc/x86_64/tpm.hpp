/*
 * Trusted Platform Module (TPM)
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

#include "memory.hpp"
#include "tpm_alg.hpp"
#include "wait.hpp"

class Tpm final : private Tcg
{
    private:
        enum class Iftype : unsigned
        {
            FIFO    = 0x0,                              //  FIFO
            CRB     = 0x1,                              //  CRB
        };

        static inline Iftype ift;

        enum class Family : unsigned
        {
            TPM_12  = 0x0,                              //  TPM 1.2
            TPM_20  = 0x1,                              //  TPM 2.0
        };

        static inline Family fam;

        enum class Locality : unsigned                  //  Use     Reset       Extend
        {
            L0      = 0x0000,                           //  LEG     16,23       0-16,23
            L1      = 0x1000,                           //  TOS     16,23       0-16,20,23
            L2      = 0x2000,                           //  MLE     16,20-23    0-23
            L3      = 0x3000,                           //  ACM     16,20-23    0-20,23
            L4      = 0x4000,                           //  TXT     16,20-23    0-18,23
        };

        class Interface
        {
            protected:
                enum class Reg8 : unsigned              //  Acc Loc Ver
                {
                    FIFO_ACCESS             = 0x000,    //  rw  N   1.2+
                    FIFO_DATA               = 0x024,    //  rw  Y   1.2+
                    CRB_DATA                = 0x080,    //  rw  Y   2.0+
                };

                enum class Reg32 : unsigned             //  Acc Loc Ver
                {
                    CRB_LOC_STATE           = 0x000,    //  r-  N   2.0+
                    CRB_LOC_CTRL            = 0x008,    //  -w  N   2.0+
                    CRB_LOC_STS             = 0x00c,    //  r-  N   2.0+
                    FIFO_INTF_CAPABILITY    = 0x014,    //  r-  N   1.2+
                    FIFO_STS                = 0x018,    //  rw  Y   1.2+
                    INTERFACE_ID            = 0x030,    //  rw  N   2.0+
                    CRB_DIDVID              = 0x034,    //  r-  N   2.0+
                    CRB_CTRL_REQ            = 0x040,    //  rw  Y   2.0+
                    CRB_CTRL_STS            = 0x044,    //  r-  Y   2.0+
                    CRB_CTRL_START          = 0x04c,    //  rw  Y   2.0+
                    FIFO_DIDVID             = 0xf00,    //  r-  N   1.2+
                };

                static auto read  (Locality l, Reg8  r)      { return *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)); }
                static auto read  (Locality l, Reg32 r)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)); }
                static void write (Locality l, Reg8  r, uint8_t  v) { *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)) = v; }
                static void write (Locality l, Reg32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)) = v; }

            public:
                static auto type() { return read (Locality::L0, Reg32::INTERFACE_ID) & BIT_RANGE (3, 0); }
        };

        // TCG PC Client Platform TPM Profile Specification for TPM 2.0, Section 6.5.2
        class Fifo final : private Interface
        {
            private:
                [[nodiscard]] static auto wait_locality (Locality l, uint8_t m)
                {
                    // Wait 750ms (TIMEOUT_A) for tpmRegValidSts=1 and activeLocality|beenSeized|requestUse = m
                    return Wait::until (750, [&] { return (read (l, Reg8::FIFO_ACCESS) & (BIT (7) | BIT (5) | BIT (4) | BIT (1))) == (BIT (7) | m); });
                }

                [[nodiscard]] static auto wait_status (Locality l, uint32_t m)
                {
                    // Wait 2000ms (TIMEOUT_B) for all m-bits to become 1
                    return Wait::until (2000, [&] { return (read (l, Reg32::FIFO_STS) & m) == m; });
                }

                [[nodiscard]] static auto wait_done (Locality l, uint32_t m)
                {
                    uint32_t s;

                    // Wait 750ms (TIMEOUT_A) for stsValid=1 and subsequently check that Expect/dataAvail is 0
                    return Wait::until (750, [&] { return (s = read (l, Reg32::FIFO_STS)) & BIT (7); }) && !(s & m);
                }

                [[nodiscard]] static auto burstcount (Locality l, unsigned &b)
                {
                    // Wait 750ms (TIMEOUT_A) for BurstCount > 0
                    return Wait::until (750, [&] { return b = static_cast<uint16_t>(read (l, Reg32::FIFO_STS) >> 8); });
                }

                [[nodiscard]] static auto state (Locality l, bool idle)
                {
                    // Set commandReady
                    write (l, Reg32::FIFO_STS, BIT (6));

                    // Wait for commandReady=1 unless going idle
                    return idle || wait_status (l, BIT (6));
                }

                [[nodiscard]] static auto exec (Locality l)
                {
                    // Set tpmGo
                    write (l, Reg32::FIFO_STS, BIT (5));

                    // Wait for stsValid=1 and dataAvail=1
                    return wait_status (l, BIT (7) | BIT (4));
                }

                [[nodiscard]] static bool send (Locality, Cmd const &);
                [[nodiscard]] static bool recv (Locality);

            public:
                static void init (uint32_t &did)
                {
                    ift = Iftype::FIFO;
                    fam = (read (Locality::L0, Reg32::FIFO_INTF_CAPABILITY) >> 28 & BIT_RANGE (2, 0)) == 3 ? Family::TPM_20 : Family::TPM_12;
                    did = read (Locality::L0, Reg32::FIFO_DIDVID);
                }

                [[nodiscard]] static bool execute (Locality, Cmd const &);
                [[nodiscard]] static auto request (Locality l) { write (l, Reg8::FIFO_ACCESS, BIT (1)); return wait_locality (l, BIT (5)); }
                [[nodiscard]] static auto release (Locality l) { write (l, Reg8::FIFO_ACCESS, BIT (5)); return wait_locality (l, 0); }
        };

        // TCG PC Client Platform TPM Profile Specification for TPM 2.0, Section 6.5.3
        class Crb final : private Interface
        {
            private:
                [[nodiscard]] static auto wait_locality (Locality l, uint32_t m)
                {
                    // Wait 750ms (TIMEOUT_A) for locality status change to m
                    return Wait::until (750, [&] { return (read (l, Reg32::CRB_LOC_STS) & BIT_RANGE (1, 0)) == m; });
                }

                [[nodiscard]] static auto state (Locality l, uint32_t m)
                {
                    // Set either cmdReady or goIdle
                    write (l, Reg32::CRB_CTRL_REQ, m);

                    // Wait 200ms (TIMEOUT_C) for the respective bit to be cleared
                    return Wait::until (200, [&] { return (read (l, Reg32::CRB_CTRL_REQ) & m) == 0; });
                }

                [[nodiscard]] static auto exec (Locality l)
                {
                    // Set Start
                    write (l, Reg32::CRB_CTRL_START, BIT (0));

                    // Wait 2000ms (TIMEOUT_B) for the respective bit to be cleared
                    return Wait::until (2000, [&] { return (read (l, Reg32::CRB_CTRL_START) & BIT (0)) == 0; });
                }

                [[nodiscard]] static bool send (Locality, Cmd const &);
                [[nodiscard]] static bool recv (Locality);

            public:
                static void init (uint32_t &did)
                {
                    ift = Iftype::CRB;
                    fam = Family::TPM_20;
                    did = read (Locality::L0, Reg32::CRB_DIDVID);
                }

                [[nodiscard]] static bool execute (Locality, Cmd const &);
                [[nodiscard]] static auto request (Locality l) { write (l, Reg32::CRB_LOC_CTRL, BIT (0)); return wait_locality (l, 1); }
                [[nodiscard]] static auto release (Locality l) { write (l, Reg32::CRB_LOC_CTRL, BIT (1)); return wait_locality (l, 0); }
        };

        [[nodiscard]] static bool execute (Locality, Cmd const &);
        [[nodiscard]] static auto request (Locality l) { return ift == Iftype::CRB ? Crb::request (l) : Fifo::request (l); }
        [[nodiscard]] static auto release (Locality l) { return ift == Iftype::CRB ? Crb::release (l) : Fifo::release (l); }

        [[nodiscard]] static bool v2_shutdown() { return execute (Locality::L0, Tpm20_shutdown { Tpm_su::Type::SU_CLEAR }); }
        [[nodiscard]] static bool v1_shutdown() { return execute (Locality::L0, Tpm12_shutdown {}); }

        [[nodiscard]] static bool v2_pcr_reset (unsigned pcr) { return execute (Locality::L2, Tpm20_pcr_reset { pcr }); }
        [[nodiscard]] static bool v1_pcr_reset (unsigned pcr) { return execute (Locality::L2, Tpm12_pcr_reset { pcr }); }

        template <typename T>
        [[nodiscard]] static bool v2_pcr_extend (unsigned pcr, T             const &hash) { return execute (Locality::L2, Tpm20_pcr_extend { pcr, hash }); }
        [[nodiscard]] static bool v1_pcr_extend (unsigned pcr, Hash_sha1_160 const &hash) { return execute (Locality::L2, Tpm12_pcr_extend { pcr, hash }); }

        static bool v2_cap_pcrs();
        static bool v2_cap_tpm_properties();
        static bool v1_cap_tpm_properties (uint32_t &, Tpm12_ptg::Type);

        static inline uint32_t tpm_mfr { 0 }, num_pcr { 0 }, max_buf { 0 }, max_dig { 0 };

        static inline uint8_t buffer[1024];

        static inline Hash_bmp hash;

    public:
        static bool init (bool);

        [[nodiscard]] static bool extend (unsigned pcr, Hash_sha1_160 const &sha1_160, Hash_sha2_256 const &sha2_256, Hash_sha2_384 const &sha2_384, Hash_sha2_512 const &sha2_512)
        {
            if (!request (Locality::L2))
                return false;

            bool complete { true };

            switch (fam) {

                case Family::TPM_12:
                    complete &= v1_pcr_extend (pcr, sha1_160);
                    break;

                case Family::TPM_20:
                    complete &= !hash.supported (Hash_bmp::Type::SHA1_160) || v2_pcr_extend (pcr, sha1_160);
                    complete &= !hash.supported (Hash_bmp::Type::SHA2_256) || v2_pcr_extend (pcr, sha2_256);
                    complete &= !hash.supported (Hash_bmp::Type::SHA2_384) || v2_pcr_extend (pcr, sha2_384);
                    complete &= !hash.supported (Hash_bmp::Type::SHA2_512) || v2_pcr_extend (pcr, sha2_512);
                    break;
            }

            return release (Locality::L2) && complete;
        }
};
