/*
 * Trusted Platform Module (TPM)
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

#include "memory.hpp"
#include "tpm_alg.hpp"
#include "wait.hpp"

class Tpm final : private Tcg
{
    public:
        enum class Locality : unsigned          //  Use     Reset       Extend
        {
            L0      = 0x0000,                   //  LEG     16,23       0-16,23
            L1      = 0x1000,                   //  TOS     16,23       0-16,20,23
            L2      = 0x2000,                   //  MLE     16,20-23    0-23
            L3      = 0x3000,                   //  ACM     16,20-23    0-20,23
            L4      = 0x4000,                   //  TXT     17-22       0-18,23
        };

    private:
        enum class Iftype : unsigned
        {
            FIFO    = 0x0,                      //  PTP: FIFO
            CRB     = 0x1,                      //  PTP: CRB
            RAM_CRB = 0x2,                      //  PTP: RAM CRB
            LEGACY  = 0xf,                      //  TIS: Legacy
        };

        template<typename T> class Interface
        {
            protected:
                enum Timeout : unsigned         // Interface Timeouts in ms (Table 27)
                {
                    A   =  750,
                    B   = 2000,
                    C   =  200,
                    D   =   30,
                };

                // Using a member function template delays type checking of the derived class T, which is not yet fully defined here
                template<typename X = T> static auto read  (Locality l, typename X::Reg8  r)      { return *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)); }
                template<typename X = T> static auto read  (Locality l, typename X::Reg32 r)      { return *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)); }
                template<typename X = T> static auto read  (Locality l, typename X::Reg64 r)      { return *reinterpret_cast<uint64_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)); }
                template<typename X = T> static void write (Locality l, typename X::Reg8  r, uint8_t  v) { *reinterpret_cast<uint8_t  volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)) = v; }
                template<typename X = T> static void write (Locality l, typename X::Reg32 r, uint32_t v) { *reinterpret_cast<uint32_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)) = v; }
                template<typename X = T> static void write (Locality l, typename X::Reg64 r, uint64_t v) { *reinterpret_cast<uint64_t volatile *>(MMAP_GLB_TPM2 + std::to_underlying (l) + std::to_underlying (r)) = v; }
        };

        // TCG PC Client Platform TPM Profile Specification for TPM 2.0, Section 6.5.2
        class Fifo final : private Interface<Fifo>
        {
            friend class Interface;

            private:
                enum class Reg8 : unsigned      //  Acc Loc Ver
                {
                    ACCESS          = 0x000,    //  rw  N   1.2+
                    DATA            = 0x024,    //  rw  Y   1.2+
                };

                enum class Reg32 : unsigned     //  Acc Loc Ver
                {
                    INTF_CAPABILITY = 0x014,    //  r-  N   1.2+
                    STS             = 0x018,    //  rw  Y   1.2+
                    INTERFACE_ID    = 0x030,    //  rw  N   2.0+
                    DID_VID         = 0xf00,    //  r-  N   1.2+
                };

                [[nodiscard]] static bool wait_locality (Locality l, uint8_t m)
                {
                    // Wait for tpmRegValidSts=1 and activeLocality|beenSeized|requestUse = m
                    return Wait::until (Timeout::A, [&] { return (read (l, Reg8::ACCESS) & (BIT (7) | BIT (5) | BIT (4) | BIT (1))) == (BIT (7) | m); });
                }

                [[nodiscard]] static bool wait_status (Locality l, uint32_t m)
                {
                    // Wait for all m-bits to become 1
                    return Wait::until (Timeout::B, [&] { return (read (l, Reg32::STS) & m) == m; });
                }

                [[nodiscard]] static bool wait_done (Locality l, uint32_t m)
                {
                    uint32_t s;

                    // Wait for stsValid=1 and check that Expect/dataAvail = 0
                    return Wait::until (Timeout::A, [&] { return (s = read (l, Reg32::STS)) & BIT (7); }) && !(s & m);
                }

                [[nodiscard]] static bool burstcount (Locality l, unsigned &b)
                {
                    // Wait for BurstCount > 0
                    return Wait::until (Timeout::A, [&] { return b = static_cast<uint16_t>(read (l, Reg32::STS) >> 8); });
                }

                [[nodiscard]] static bool state (Locality l, bool idle)
                {
                    // Set commandReady
                    write (l, Reg32::STS, BIT (6));

                    // Wait for commandReady = 1 unless going idle
                    return idle || wait_status (l, BIT (6));
                }

                [[nodiscard]] static bool exec (Locality l)
                {
                    // Set tpmGo
                    write (l, Reg32::STS, BIT (5));

                    // Wait for stsValid = 1 and dataAvail = 1
                    return wait_status (l, BIT (7) | BIT (4));
                }

                [[nodiscard]] static bool recv (Locality l)
                {
                    auto p { buffer };
                    auto s { sizeof (Res) };

                    // Read response
                    for (unsigned i { 0 }, b { 0 }; i < s; b--) {

                        // Update burstcount whenever it is 0
                        if (!b && !burstcount (l, b)) [[unlikely]]
                            return false;

                        *p++ = read (l, Reg8::DATA);

                        if (++i == sizeof (Res)) [[unlikely]] {

                            // Determine total response size from header
                            s = reinterpret_cast<Res const *>(buffer)->size();

                            // Fail if the response does not fit into the receive buffer
                            if (s > sizeof (buffer)) [[unlikely]]
                                return false;
                        }
                    }

                    // Check for complete transmission
                    return wait_done (l, BIT (4));
                }

                [[nodiscard]] static bool send (Locality l, Cmd const &c)
                {
                    auto p { reinterpret_cast<uint8_t const *>(&c) };
                    auto s { c.size() };

                    // Write command
                    for (unsigned i { 0 }, b { 0 }; i < s; b--, i++) {

                        // Update burstcount whenever it is 0
                        if (!b && !burstcount (l, b)) [[unlikely]]
                            return false;

                        write (l, Reg8::DATA, *p++);
                    }

                    // Check for complete transmission
                    return wait_done (l, BIT (3));
                }

            public:
                [[nodiscard]] static bool init (uint32_t &did_vid)
                {
                    did_vid = read (Locality::L0, Reg32::DID_VID);

                    /*
                     * INTF_CAPABILITY[30:28] provides family identification (especially if INTERFACE_ID[3:0] is 0b1111)
                     * - 0b000 Family 1.2, FIFO Interface 1.21 or earlier
                     * - 0b010 Family 1.2, FIFO Interface 1.3
                     * - 0b011 Family 2.0, FIFO Interface 1.3
                     * STS[27:26] could also provide family identification, but requires locality to be active
                     */
                    return (read (Locality::L0, Reg32::INTF_CAPABILITY) >> 28 & BIT_RANGE (2, 0)) == 3;
                }

                [[nodiscard]] static bool execute (Locality l, Cmd const &c)
                {
                    // Enter ready state
                    if (!state (l, false)) [[unlikely]]
                        return false;

                    bool const ret { send (l, c) && exec (l) && recv (l) };

                    // Enter idle state
                    return state (l, true) && ret;
                }

                [[nodiscard]] static bool request (Locality l) { write (l, Reg8::ACCESS, BIT (1)); return wait_locality (l, BIT (5)); }
                [[nodiscard]] static bool release (Locality l) { write (l, Reg8::ACCESS, BIT (5)); return wait_locality (l, 0); }

                [[nodiscard]] static auto iftype() { return Iftype { read (Locality::L0, Reg32::INTERFACE_ID) & BIT_RANGE (3, 0) }; }
        };

        // TCG PC Client Platform TPM Profile Specification for TPM 2.0, Section 6.5.3
        class Crb final : private Interface<Crb>
        {
            friend class Interface;

            private:
                enum class Reg8 : unsigned      //  Acc Loc Ver
                {
                    DATA            = 0x080,    //  rw  Y   2.0+
                };

                enum class Reg32 : unsigned     //  Acc Loc Ver
                {
                    LOC_STATE       = 0x000,    //  r-  N   2.0+
                    LOC_CTRL        = 0x008,    //  -w  N   2.0+
                    LOC_STS         = 0x00c,    //  r-  N   2.0+
                    CTRL_REQ        = 0x040,    //  rw  Y   2.0+
                    CTRL_STS        = 0x044,    //  r-  Y   2.0+
                    CTRL_START      = 0x04c,    //  rw  Y   2.0+
                };

                enum class Reg64 : unsigned     //  Acc Loc Ver
                {
                    INTERFACE_ID    = 0x030,    //  rw  N   2.0+
                };

                [[nodiscard]] static bool wait_locality (Locality l, uint32_t m)
                {
                    // Wait for beenSeized|Granted = m
                    return Wait::until (Timeout::A, [&] { return (read (l, Reg32::LOC_STS) & BIT_RANGE (1, 0)) == m; });
                }

                [[nodiscard]] static bool state (Locality l, uint32_t m)
                {
                    // Set either cmdReady or goIdle
                    write (l, Reg32::CTRL_REQ, m);

                    // Wait for the respective bit to be cleared
                    return Wait::until (Timeout::C, [&] { return (read (l, Reg32::CTRL_REQ) & m) == 0; });
                }

                [[nodiscard]] static bool exec (Locality l)
                {
                    // Set Start
                    write (l, Reg32::CTRL_START, BIT (0));

                    // Wait for the respective bit to be cleared
                    return Wait::until (Timeout::B, [&] { return (read (l, Reg32::CTRL_START) & BIT (0)) == 0; });
                }

                [[nodiscard]] static bool recv (Locality l)
                {
                    auto p { buffer };
                    auto s { sizeof (Res) };

                    // Read response
                    for (unsigned i { 0 }; i < s; ) {

                        *p++ = read (l, Reg8 { std::to_underlying (Reg8::DATA) + i });

                        if (++i == sizeof (Res)) [[unlikely]] {

                            // Determine total response size from header
                            s = reinterpret_cast<Res const *>(buffer)->size();

                            // Fail if the response does not fit into the receive buffer
                            if (s > sizeof (buffer)) [[unlikely]]
                                return false;
                        }
                    }

                    return true;
                }

                [[nodiscard]] static bool send (Locality l, Cmd const &c)
                {
                    auto p { reinterpret_cast<uint8_t const *>(&c) };
                    auto s { c.size() };

                    // Write command
                    for (unsigned i { 0 }; i < s; i++)
                        write (l, Reg8 { std::to_underlying (Reg8::DATA) + i }, *p++);

                    return true;
                }

            public:
                [[nodiscard]] static bool init (uint32_t &did_vid)
                {
                    auto const i { read (Locality::L0, Reg64::INTERFACE_ID) };

                    did_vid = static_cast<uint32_t>(i >> 32);

                    return (i >> 4 & BIT_RANGE (3, 0)) > 0;
                }

                [[nodiscard]] static bool execute (Locality l, Cmd const &c)
                {
                    // Enter ready state
                    if (!state (l, BIT (0))) [[unlikely]]
                        return false;

                    bool const ret { send (l, c) && exec (l) && recv (l) };

                    // Enter idle state
                    return state (l, BIT (1)) && ret;
                }

                [[nodiscard]] static bool request (Locality l) { write (l, Reg32::LOC_CTRL, BIT (0)); return wait_locality (l, 1); }
                [[nodiscard]] static bool release (Locality l) { write (l, Reg32::LOC_CTRL, BIT (1)); return wait_locality (l, 0); }
        };

        // Interface Dispatch Function Pointers
        static inline constinit bool (*f_request)(Locality)              { [](Locality)              { return false; } };
        static inline constinit bool (*f_release)(Locality)              { [](Locality)              { return false; } };
        static inline constinit bool (*f_execute)(Locality, Cmd const &) { [](Locality, Cmd const &) { return false; } };

        // Interface Dispatch Functions
        [[nodiscard]] static bool request (Locality l)               { return f_request (l); }
        [[nodiscard]] static bool release (Locality l)               { return f_release (l); }
        [[nodiscard]] static bool execute (Locality l, Cmd const &c) { return f_execute (l, c); }

        [[nodiscard]] static bool invoke (Locality, Cmd &&, Hmac_session * = nullptr);

        [[nodiscard]] static bool shutdown (Locality l) { return invoke (l, Tpm2_shutdown { Tpm_su::Type::CLEAR }); }
        [[nodiscard]] static bool flush_context (Locality l, Tpm_handle h) { return invoke (l, Tpm2_flush_context { h }); }
        [[nodiscard]] static bool pcr_reset (Locality l, Hmac_session &s, unsigned pcr) { return invoke (l, Tpm2_pcr_reset<Hmac_session::Type> { pcr }, &s); }
        [[nodiscard]] static bool pcr_extend (Locality l, Hmac_session &s, unsigned pcr, auto const &hash) { return invoke (l, Tpm2_pcr_extend<Hmac_session::Type> { pcr, hash }, &s); }

        [[nodiscard]] static bool start_auth_session (Locality, Hmac_session &);
        [[nodiscard]] static bool cap_pcrs (Locality);
        [[nodiscard]] static bool cap_tpm_properties (Locality);

        static inline constinit uint32_t tpm_rev {}, ver_fw1 {}, ver_fw2 {}, num_pcr {}, max_cmd {}, max_res {};
        static inline constinit char tpm_mfr[4] {}, tpm_ven[16] {};

        static inline constinit uint8_t buffer[1024];

        static inline constinit Hash_bmp hash;

    public:
        static bool init (bool);

        [[nodiscard]] static bool extend (Locality l, unsigned pcr, Hash_sha1_160 const &sha1_160, Hash_sha2_256 const &sha2_256, Hash_sha2_384 const &sha2_384, Hash_sha2_512 const &sha2_512)
        {
            // Request locality
            if (!request (l)) [[unlikely]]
                return false;

            Hmac_session s { Tpma_session { Tpma_session::CONTINUE_SESSION } };

            // Start authorization session
            if (!start_auth_session (l, s)) [[unlikely]]
                return release (l) && false;

            auto const ret
            {
                (!hash.supported (Hash_bmp::Type::SHA1_160) || pcr_extend (l, s, pcr, sha1_160)) &
                (!hash.supported (Hash_bmp::Type::SHA2_256) || pcr_extend (l, s, pcr, sha2_256)) &
                (!hash.supported (Hash_bmp::Type::SHA2_384) || pcr_extend (l, s, pcr, sha2_384)) &
                (!hash.supported (Hash_bmp::Type::SHA2_512) || pcr_extend (l, s, pcr, sha2_512)) &
                flush_context (l, s.handle)
            };

            // Release locality
            return release (l) && ret;
        }
};
