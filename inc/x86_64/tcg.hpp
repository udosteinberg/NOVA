/*
 * Trusted Computing Group (TCG) Definitions
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

#include "drng.hpp"
#include "hmac.hpp"
#include "string.hpp"

class Tcg
{
    private:
        // Typed Size
        template<typename T> struct Size : private Unaligned_be<T>
        {
            auto size() const { return T { *this }; }

            explicit constexpr Size (T v) : Unaligned_be<T> { v } {}
        };

        // Typed List
        template<typename T> struct Tpml : public Size<uint32_t>
        {
            auto next() const { return std::start_lifetime_as<T const> (this + 1); }

            explicit constexpr Tpml (uint32_t s) : Size { s } {}
        };

        // PCR Bitmap
        template<unsigned N> struct Pcr_bitmap final
        {
            uint8_t pcrs[N];

            explicit constexpr Pcr_bitmap (uint64_t v)
            {
                for (size_t i { 0 }; i < sizeof (pcrs); v >>= 8)
                    pcrs[i++] = static_cast<uint8_t>(v);
            }
        };

    public:
        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 11: Definition of (UINT16) TPM_ALG_ID Constants
         */
        struct Tpm_alg_id final : private Unaligned_be<uint16_t>
        {
            enum class Hash : uint16_t      // Table 78: TPMI_ALG_HASH Type
            {
                SHA1_160                    = 0x0004,
                SHA2_256                    = 0x000b,
                SHA2_384                    = 0x000c,
                SHA2_512                    = 0x000d,
                SM3_256                     = 0x0012,
                SHA3_256                    = 0x0027,
                SHA3_384                    = 0x0028,
                SHA3_512                    = 0x0029,
            };

            enum class Sym : uint16_t       // Table 80: TPMI_ALG_SYM Type
            {
                XOR                         = 0x000a,
                NULL                        = 0x0010,
            };

            auto hash() const { return Hash { uint16_t { *this } }; }

            explicit constexpr Tpm_alg_id (Hash t) : Unaligned_be<uint16_t> { std::to_underlying (t) } {}
            explicit constexpr Tpm_alg_id (Sym  t) : Unaligned_be<uint16_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_alg_id) == 1 && sizeof (Tpm_alg_id) == sizeof (uint16_t));

    protected:
        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 14: Definition of (UINT32) TPM_CC Constants
         */
        struct Tpm_cc : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t      // Upper 16 bits are zero
            {
                PCR_RESET                   = 0x013d,
                SHUTDOWN                    = 0x0145,
                FLUSH_CONTEXT               = 0x0165,
                START_AUTH_SESSION          = 0x0176,
                GET_CAPABILITY              = 0x017a,
                PCR_EXTEND                  = 0x0182,
            };

            auto cc() const { return Type { uint32_t { *this } }; }

            explicit constexpr Tpm_cc (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_cc) == 1 && sizeof (Tpm_cc) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 20: Definition of (UINT32) TPM_RC Constants
         */
        struct Tpm_rc : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                SUCCESS                     = 0x000,
                FAILURE                     = 0x101,
            };

            auto rc() const { return Type { uint32_t { *this } & (uint32_t { *this } & BIT (7) ? 0xbf : 0xfff) }; }
        };

        static_assert (alignof (Tpm_rc) == 1 && sizeof (Tpm_rc) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 23: Definition of (UINT16) TPM_ST Constants
         */
        struct Tpm_st : private Unaligned_be<uint16_t>
        {
            enum class Type : uint16_t
            {
                NO_SESSIONS                 = 0x8001,
                SESSIONS                    = 0x8002,
            };

            auto st() const { return Type { uint16_t { *this } }; }

            explicit constexpr Tpm_st (Type t) : Unaligned_be<uint16_t> { std::to_underlying (t) } {}

            struct Meta { uint8_t cmd, res; };

            auto h_cmd() const { return static_cast<uint8_t>(uint16_t { *this } >> 8); }
            auto h_res() const { return static_cast<uint8_t>(uint16_t { *this } >> 0); }

            explicit constexpr Tpm_st (Meta m) : Unaligned_be<uint16_t> { static_cast<uint16_t>(m.cmd << 8 | m.res) } {}
        };

        static_assert (alignof (Tpm_st) == 1 && sizeof (Tpm_st) == sizeof (uint16_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 24: Definition of (UINT16) TPM_SU Constants
         */
        struct Tpm_su final : private Unaligned_be<uint16_t>
        {
            enum class Type : uint16_t
            {
                CLEAR                       = 0x0000,
                STATE                       = 0x0001,
            };

            explicit constexpr Tpm_su (Type t) : Unaligned_be<uint16_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_su) == 1 && sizeof (Tpm_su) == sizeof (uint16_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 25: Definition of (UINT8) TPM_SE Constants
         */
        struct Tpm_se final : private Unaligned_be<uint8_t>
        {
            enum class Type : uint8_t
            {
                HMAC                        = 0x00,
                POLICY                      = 0x01,
                TRIAL                       = 0x03,
            };

            explicit constexpr Tpm_se (Type t) : Unaligned_be<uint8_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_se) == 1 && sizeof (Tpm_se) == sizeof (uint8_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 26: Definition of (UINT32) TPM_CAP Constants
         */
        struct Tpm_cap final : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                PCRS                        = 0x5,
                TPM_PROPERTIES              = 0x6,
            };

            auto type() const { return Type { uint32_t { *this } }; }

            explicit constexpr Tpm_cap (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_cap) == 1 && sizeof (Tpm_cap) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 30: Definition of (UINT32) TPM_PT Constants
         */
        struct Tpm_pt final : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                REVISION                    = 0x100 + 2,
                MANUFACTURER                = 0x100 + 5,
                VENDOR_STRING_1             = 0x100 + 6,
                VENDOR_STRING_2             = 0x100 + 7,
                VENDOR_STRING_3             = 0x100 + 8,
                VENDOR_STRING_4             = 0x100 + 9,
                FIRMWARE_VERSION_1          = 0x100 + 11,
                FIRMWARE_VERSION_2          = 0x100 + 12,
                PCR_COUNT                   = 0x100 + 18,
                MAX_COMMAND_SIZE            = 0x100 + 30,
                MAX_RESPONSE_SIZE           = 0x100 + 31,
            };

            auto type() const { return Type { uint32_t { *this } }; }

            explicit constexpr Tpm_pt (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_pt) == 1 && sizeof (Tpm_pt) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 34: Definition of Types for Handles
         */
        struct Tpm_handle final : private Unaligned_be<uint32_t>
        {
            // Table 35: Definition of TPM_HT Constants
            enum class Type : uint8_t
            {
                PCR                         = 0x00,
                PERMANENT                   = 0x40,
            };

            // Table 36: Definition of TPM_RH Constants
            enum class Rh : uint32_t
            {
                NULL                        = 0x7,
                PW                          = 0x9,
            };

            auto raw() const { return uint32_t { *this }; }

            explicit constexpr Tpm_handle() = default;

            // Constructor: Generic Handle
            explicit constexpr Tpm_handle (Type t, uint32_t n) : Unaligned_be<uint32_t> { std::to_underlying (t) << 24 | n } {}

            // Constructor: Permanent Handle
            explicit constexpr Tpm_handle (Rh t) : Tpm_handle { Type::PERMANENT, std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_handle) == 1 && sizeof (Tpm_handle) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 40: Definition of (UINT8) TPMA_SESSION Bits
         */
        struct Tpma_session final : private Unaligned_be<uint8_t>
        {
            enum
            {
                CONTINUE_SESSION    = BIT (0),
                AUDIT_EXCLUSIVE     = BIT (1),
                AUDIT_RESET         = BIT (2),
                DECRYPT             = BIT (5),
                ENCRYPT             = BIT (6),
                AUDIT               = BIT (7),
            };

            explicit constexpr Tpma_session() = default;

            explicit constexpr Tpma_session (uint8_t a) : Unaligned_be<uint8_t> { a } {}
        };

        static_assert (alignof (Tpma_session) == 1 && sizeof (Tpma_session) == sizeof (uint8_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 92: Definition of TPM2B_* Structure
         */
        struct Tpm2b final : public Size<uint16_t>                                  // Size is flexible
        {
            auto data() const { return reinterpret_cast<uint8_t const *>(this + 1); }
            auto next() const { return std::start_lifetime_as<Tpm2b const> (data() + size()); }
        };

        static_assert (alignof (Tpm2b) == 1 && sizeof (Tpm2b) == sizeof (uint16_t));

        template<unsigned S> struct Tpm2b_digest : public Size<uint16_t>
        {
            uint8_t dig[S] {};

            explicit constexpr Tpm2b_digest() : Size { S } {}
            explicit constexpr Tpm2b_digest (uint8_t const (&d)[S]) : Size { S } { __builtin_memcpy (dig, d, S); }
        };

        static_assert (alignof (Tpm2b_digest<0>) == 1 && sizeof (Tpm2b_digest<0>) == sizeof (uint16_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 105: Definition of TPMS_PCR_SELECT Structure
         */
        struct Tpms_pcr_select final : private Size<uint8_t>                        // Size is flexible
        {
            auto pbmp() const { return reinterpret_cast<uint8_t const *>(this + 1); }
            auto next() const { return pbmp() + size(); }

            auto pcrs() const
            {
                uint64_t v { 0 };
                for (auto i { size() }; i--; v = v << 8 | pbmp()[i]) ;
                return v;
            }

            explicit constexpr Tpms_pcr_select (uint8_t s) : Size { s } {}
        };

        static_assert (alignof (Tpms_pcr_select) == 1 && sizeof (Tpms_pcr_select) == sizeof (uint8_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 106: Definition of TPMS_PCR_SELECTION Structure
         */
        struct Tpms_pcr_selection final                                             // Size is flexible because of Tpms_pcr_select
        {
            Tpm_alg_id              const alg;                                      // Hash Algorithm
            Tpms_pcr_select         const sel;                                      // PCR Selection

            auto next() const { return std::start_lifetime_as<Tpms_pcr_selection const> (sel.next()); }

            explicit constexpr Tpms_pcr_selection (Tpm_alg_id::Hash a, uint8_t s) : alg { a }, sel { s } {}
        };

        static_assert (alignof (Tpms_pcr_selection) == 1 && sizeof (Tpms_pcr_selection) == sizeof (Tpm_alg_id) + sizeof (Tpms_pcr_select));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 114: Definition of TPMS_TAGGED_PROPERTY Structure
         */
        struct Tpms_tagged_property final
        {
            Tpm_pt                  const ptg;                                      // Property Tag
            Unaligned_be<uint32_t>  const val;                                      // Property Value

            auto next() const { return this + 1; }
        };

        static_assert (alignof (Tpms_tagged_property) == 1 && sizeof (Tpms_tagged_property) == sizeof (Tpm_pt) + sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 123: Definition of TPML_DIGEST Structure
         */
        struct Tpml_digest final : public Tpml<Tpm2b> {};

        static_assert (alignof (Tpml_digest) == 1 && sizeof (Tpml_digest) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 124: Definition of TPML_DIGEST_VALUES Structure
         */
        struct Tpml_digest_values final : public Tpml<Tpm_alg_id>
        {
            explicit constexpr Tpml_digest_values (uint32_t s) : Tpml { s } {}
        };

        static_assert (alignof (Tpml_digest_values) == 1 && sizeof (Tpml_digest_values) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 125: Definition of TPML_PCR_SELECTION Structure
         */
        struct Tpml_pcr_selection final : public Tpml<Tpms_pcr_selection>
        {
            explicit constexpr Tpml_pcr_selection (uint32_t s) : Tpml { s } {}
        };

        static_assert (alignof (Tpml_pcr_selection) == 1 && sizeof (Tpml_pcr_selection) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 127: Definition of TPML_TAGGED_TPM_PROPERTY Structure
         */
        struct Tpml_tagged_tpm_property final : public Tpml<Tpms_tagged_property> {};

        static_assert (alignof (Tpml_tagged_tpm_property) == 1 && sizeof (Tpml_tagged_tpm_property) == sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 136: Definition of TPMS_CAPABILITY_DATA Structure
         */
        struct Tpms_capability_data final
        {
            Tpm_cap const cap;

            template<typename T> auto next() const { return std::start_lifetime_as<T const> (this + 1); }
        };

        static_assert (alignof (Tpms_capability_data) == 1 && sizeof (Tpms_capability_data) == sizeof (Tpm_cap));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 153: Definition of TPMS_AUTH_COMMAND Structure
         */
        template<typename H> struct Tpms_auth_command final
        {
            Tpm_handle                  handle;     // Session Handle
            Tpm2b_digest<H::dig_size>   nonce;      // Session Nonce    (+Size)
            Tpma_session                attr;       // Session Attributes
            Tpm2b_digest<H::dig_size>   hmac;       // Session Auth     (+Size)
        };

        static_assert (alignof (Tpms_auth_command<Hash_sha2_256>) == 1 && sizeof (Tpms_auth_command<Hash_sha2_256>) == sizeof (Tpm_handle) + sizeof (Tpma_session) + 2 * sizeof (Tpm2b_digest<Hash_sha2_256::dig_size>));
        static_assert (alignof (Tpms_auth_command<Hash_sha2_512>) == 1 && sizeof (Tpms_auth_command<Hash_sha2_512>) == sizeof (Tpm_handle) + sizeof (Tpma_session) + 2 * sizeof (Tpm2b_digest<Hash_sha2_512::dig_size>));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 2), Table 154: Definition of TPMS_AUTH_RESPONSE Structure
         */
        template<typename H> struct Tpms_auth_response final
        {
            Tpm2b_digest<H::dig_size>   nonce;      // Session Nonce    (+Size)
            Tpma_session                attr;       // Session Attributes
            Tpm2b_digest<H::dig_size>   hmac;       // Session Auth     (+Size)
        };

        static_assert (alignof (Tpms_auth_response<Hash_sha2_256>) == 1 && sizeof (Tpms_auth_response<Hash_sha2_256>) == sizeof (Tpma_session) + 2 * sizeof (Tpm2b_digest<Hash_sha2_256::dig_size>));
        static_assert (alignof (Tpms_auth_response<Hash_sha2_512>) == 1 && sizeof (Tpms_auth_response<Hash_sha2_512>) == sizeof (Tpma_session) + 2 * sizeof (Tpm2b_digest<Hash_sha2_512::dig_size>));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 1), Table 7: Command/Response Header Structure
         */
        struct Hdr : public Tpm_st, public Size<uint32_t>
        {
            explicit constexpr Hdr (uint32_t s, Tpm_st::Meta m) : Tpm_st { m }, Size { s } {}
        };

        static_assert (alignof (Hdr) == 1 && sizeof (Hdr) == sizeof (Tpm_st) + sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 1), Table 7: Command/Response Header Structure
         */
        struct Cmd : public Hdr, public Tpm_cc
        {
            explicit constexpr Cmd (uint32_t s, Tpm_st::Meta m, Tpm_cc::Type c) : Hdr { s, m }, Tpm_cc { c } {}
        };

        static_assert (alignof (Cmd) == 1 && sizeof (Cmd) == sizeof (Hdr) + sizeof (Tpm_cc));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 1), Table 7: Command/Response Header Structure
         */
        struct Res final : public Hdr, public Tpm_rc {};

        static_assert (alignof (Res) == 1 && sizeof (Res) == sizeof (Hdr) + sizeof (Tpm_rc));

        /*
         * Handle Area
         */
        template<unsigned N> class Handles
        {
            private:
                Tpm_handle const handle[N];

            public:
                template<std::same_as<Tpm_handle>... T> explicit constexpr Handles (T... h) : handle { h... } {}
        };

        /*
         * TPM 2.0 Library Specification v1.84 (Part 3), Table 6: TPM2_Shutdown Command
         */
        class Tpm2_shutdown final : public Cmd
        {
            private:
                Tpm_su const su;

            public:
                explicit constexpr Tpm2_shutdown (Tpm_su::Type t) : Cmd { sizeof (*this), { 0, 0 }, Tpm_cc::Type::SHUTDOWN }, su { t } {}
        };

        static_assert (alignof (Tpm2_shutdown) == 1 && sizeof (Tpm2_shutdown) == sizeof (Cmd) + sizeof (Tpm_su));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 3), Table 14: TPM2_StartAuthSession Command
         */
        template<typename H> class Tpm2_start_auth_session final : public Cmd, private Handles<2>
        {
            private:
                Tpm2b_digest<H::dig_size> const nonce;
                Tpm2b_digest<0>           const salt;
                Tpm_se                    const type    { Tpm_se::Type::HMAC };
                Tpm_alg_id                const sym     { Tpm_alg_id::Sym::NULL };
                Tpm_alg_id                const hash    { Tpm_alg_id::Hash { H::tpm_hash } };

            public:
                explicit constexpr Tpm2_start_auth_session (uint8_t const (&n)[H::dig_size]) : Cmd { sizeof (*this), { 2, 1 }, Tpm_cc::Type::START_AUTH_SESSION }, Handles { Tpm_handle { Tpm_handle::Rh::NULL }, Tpm_handle { Tpm_handle::Rh::NULL } }, nonce { n } {}
        };

        static_assert (alignof (Tpm2_start_auth_session<Hash_sha2_256>) == 1 && sizeof (Tpm2_start_auth_session<Hash_sha2_256>) == sizeof (Cmd) + sizeof (Handles<2>) + sizeof (Tpm2b_digest<Hash_sha2_256::dig_size>) + sizeof (Tpm2b_digest<0>) + sizeof (Tpm_se) + 2 * sizeof (Tpm_alg_id));
        static_assert (alignof (Tpm2_start_auth_session<Hash_sha2_512>) == 1 && sizeof (Tpm2_start_auth_session<Hash_sha2_512>) == sizeof (Cmd) + sizeof (Handles<2>) + sizeof (Tpm2b_digest<Hash_sha2_512::dig_size>) + sizeof (Tpm2b_digest<0>) + sizeof (Tpm_se) + 2 * sizeof (Tpm_alg_id));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 3), Table 113: TPM2_PCR_Extend Command
         */
        template<typename H> class Tpm2_pcr_extend final : public Cmd, private Handles<1>
        {
            private:
                Unaligned_be<uint32_t>    const size    { sizeof (auth) };                              // Auth Size
                Tpms_auth_command<H>            auth;                                                   // Auth #1 (User)
                Tpml_digest_values        const tpml    { 1 };                                          // Number of Digests
                Tpm_alg_id                const halg;                                                   // Hash Algorithm
                union {                                                                                 // Hash Digest
                    uint8_t                     sha1_160[Hash_sha1_160::dig_size];
                    uint8_t                     sha2_256[Hash_sha2_256::dig_size];
                    uint8_t                     sha2_384[Hash_sha2_384::dig_size];
                    uint8_t                     sha2_512[Hash_sha2_512::dig_size];
                } digest;

            public:
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha1_160 const &h) : Cmd { sizeof (*this) - sizeof (digest) + sizeof (digest.sha1_160), { 1, 0 }, Tpm_cc::Type::PCR_EXTEND }, Handles { Tpm_handle { Tpm_handle::Type::PCR, pcr } }, halg { Tpm_alg_id::Hash::SHA1_160 } { h.digest (digest.sha1_160); }
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha2_256 const &h) : Cmd { sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_256), { 1, 0 }, Tpm_cc::Type::PCR_EXTEND }, Handles { Tpm_handle { Tpm_handle::Type::PCR, pcr } }, halg { Tpm_alg_id::Hash::SHA2_256 } { h.digest (digest.sha2_256); }
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha2_384 const &h) : Cmd { sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_384), { 1, 0 }, Tpm_cc::Type::PCR_EXTEND }, Handles { Tpm_handle { Tpm_handle::Type::PCR, pcr } }, halg { Tpm_alg_id::Hash::SHA2_384 } { h.digest (digest.sha2_384); }
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha2_512 const &h) : Cmd { sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_512), { 1, 0 }, Tpm_cc::Type::PCR_EXTEND }, Handles { Tpm_handle { Tpm_handle::Type::PCR, pcr } }, halg { Tpm_alg_id::Hash::SHA2_512 } { h.digest (digest.sha2_512); }
        };

        static_assert (alignof (Tpm2_pcr_extend<Hash_sha2_256>) == 1 && sizeof (Tpm2_pcr_extend<Hash_sha2_256>) == sizeof (Cmd) + sizeof (Handles<1>) + sizeof (uint32_t) + sizeof (Tpms_auth_command<Hash_sha2_256>) + sizeof (Tpml_digest_values) + sizeof (Tpm_alg_id) + Hash_sha2_512::dig_size);
        static_assert (alignof (Tpm2_pcr_extend<Hash_sha2_512>) == 1 && sizeof (Tpm2_pcr_extend<Hash_sha2_512>) == sizeof (Cmd) + sizeof (Handles<1>) + sizeof (uint32_t) + sizeof (Tpms_auth_command<Hash_sha2_512>) + sizeof (Tpml_digest_values) + sizeof (Tpm_alg_id) + Hash_sha2_512::dig_size);

        /*
         * TPM 2.0 Library Specification v1.84 (Part 3), Table 125: TPM2_PCR_Reset Command
         */
        template<typename H> class Tpm2_pcr_reset final : public Cmd, private Handles<1>
        {
            private:
                Unaligned_be<uint32_t>    const size    { sizeof (auth) };                              // Auth Size
                Tpms_auth_command<H>            auth;                                                   // Auth #1 (User)

            public:
                explicit constexpr Tpm2_pcr_reset (unsigned pcr) : Cmd { sizeof (*this), { 1, 0 }, Tpm_cc::Type::PCR_RESET }, Handles { Tpm_handle { Tpm_handle::Type::PCR, pcr } } {}
        };

        static_assert (alignof (Tpm2_pcr_reset<Hash_sha2_256>) == 1 && sizeof (Tpm2_pcr_reset<Hash_sha2_256>) == sizeof (Cmd) + sizeof (Handles<1>) + sizeof (uint32_t) + sizeof (Tpms_auth_command<Hash_sha2_256>));
        static_assert (alignof (Tpm2_pcr_reset<Hash_sha2_512>) == 1 && sizeof (Tpm2_pcr_reset<Hash_sha2_512>) == sizeof (Cmd) + sizeof (Handles<1>) + sizeof (uint32_t) + sizeof (Tpms_auth_command<Hash_sha2_512>));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 3), Table 211: TPM2_FlushContext Command
         */
        class Tpm2_flush_context final : public Cmd
        {
            private:
                Tpm_handle                const handle;                                                 // Flush Handle

            public:
                explicit constexpr Tpm2_flush_context (Tpm_handle h) : Cmd { sizeof (*this), { 0, 0 }, Tpm_cc::Type::FLUSH_CONTEXT }, handle { h } {}
        };

        static_assert (alignof (Tpm2_flush_context) == 1 && sizeof (Tpm2_flush_context) == sizeof (Cmd) + sizeof (Tpm_handle));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 3), Table 221: TPM2_GetCapability Command
         */
        class Tpm2_get_capability final : public Cmd
        {
            private:
                Tpm_cap                   const cap;                                                    // Capability
                Tpm_pt                    const ptg;                                                    // Property Tag
                Unaligned_be<uint32_t>    const cnt;                                                    // Number of Properties

            public:
                explicit constexpr Tpm2_get_capability (Tpm_cap::Type c, Tpm_pt::Type p, uint32_t n) : Cmd { sizeof (*this), { 0, 0 }, Tpm_cc::Type::GET_CAPABILITY }, cap { c }, ptg { p }, cnt { n } {}
        };

        static_assert (alignof (Tpm2_get_capability) == 1 && sizeof (Tpm2_get_capability) == sizeof (Cmd) + sizeof (Tpm_cap) + sizeof (Tpm_pt) + sizeof (uint32_t));

        /*
         * TPM 2.0 Library Specification v1.84 (Part 1) 17.6: Session-Based Authorizations
         */
        template<typename H> struct Session final
        {
            using Type = H;

            uint8_t         phash[H::dig_size];     // pHash
            uint8_t         newer[H::dig_size];     // Nonce (Newer)
            uint8_t         older[H::dig_size];     // Nonce (Older)
            Tpma_session    attr;                   // Session Attributes
            Tpm_handle      handle;                 // Session Handle
            Hmac<H> const   key;                    // Session Key + AuthValue

            explicit constexpr Session (Tpma_session a, uint8_t const *k = nullptr, size_t s = 0) : attr { a }, key { k, s } { rand_nonce (older); }

            void rand_nonce (uint8_t (&n)[H::dig_size])
            {
                Drng::rand (reinterpret_cast<uint64_t(&)[]>(n), sizeof (n) / sizeof (uint64_t));
            }

            void undo_nonce()
            {
                __builtin_memcpy (newer, older, H::dig_size);
            }

            /*
             * Compute authorization HMAC
             */
            void compute_hmac (uint8_t (&hmac)[H::dig_size], uint8_t const (&nonce)[H::dig_size], uint8_t const *buffer, size_t size)
            {
                // Roll nonces
                __builtin_memcpy (older, newer, H::dig_size);
                __builtin_memcpy (newer, nonce, H::dig_size);

                // Compute pHash
                H {}.update (buffer, size).digest (phash);

                // TPM 2.0 Library Specification v1.84 (Part 1) 17.6.5: HMAC Computation
                key.compute (hmac, reinterpret_cast<uint8_t const *>(this), sizeof (phash) + sizeof (newer) + sizeof (older) + sizeof (attr));
            }

            /*
             * Compute command authorization
             */
            void compute_auth (Cmd *c, unsigned n)
            {
                // Unmarshal into Handle Area, Authorization Area, Parameter Area
                auto const ha { reinterpret_cast<uint8_t *>(c + 1) };
                auto const hs { n * sizeof (Tpm_handle) };
                auto const aa { ha + hs + sizeof (uint32_t) };
                auto const as { uint32_t { *std::start_lifetime_as<Unaligned_be<uint32_t> const> (aa - sizeof (uint32_t)) } };
                auto const pa { aa + as };
                auto const ps { reinterpret_cast<uint8_t const *>(c) + c->size() - pa };

                // TPM 2.0 Library Specification v1.84 (Part 1) 16.7: cp = CC || Handles || Parameters
                uint8_t cp[sizeof (Tpm_cc) + hs + ps];
                __builtin_memcpy (cp, static_cast<Tpm_cc const *>(c), sizeof (Tpm_cc) + hs);
                __builtin_memcpy (cp + sizeof (Tpm_cc) + hs, pa, ps);

                // Update Auth from Session
                auto const auth { std::start_lifetime_as<Tpms_auth_command<H>> (aa) };
                auth->handle = handle;
                auth->attr   = attr;

                // Pick a new random nonce
                rand_nonce (auth->nonce.dig);

                // Compute HMAC (HASH (cp))
                compute_hmac (auth->hmac.dig, auth->nonce.dig, cp, sizeof (cp));
            }

            /*
             * Verify response authorization
             */
            [[nodiscard]] bool verify_auth (Res const *r, unsigned n, Tpm_cc cc)
            {
                // Unmarshal into Handle Area, Parameter Area
                auto const ha { reinterpret_cast<uint8_t const *>(r + 1) };
                auto const hs { n * sizeof (Tpm_handle) };
                auto const pa { ha + hs + sizeof (uint32_t) };
                auto const ps { uint32_t { *std::start_lifetime_as<Unaligned_be<uint32_t> const> (pa - sizeof (uint32_t)) } };

                // TPM 2.0 Library Specification v1.84 (Part 1) 16.8: rp = RC || CC || Parameters
                uint8_t rp[sizeof (Tpm_rc) + sizeof (Tpm_cc) + ps];
                __builtin_memcpy (rp, static_cast<Tpm_rc const *>(r), sizeof (Tpm_rc));
                __builtin_memcpy (rp + sizeof (Tpm_rc), &cc, sizeof (Tpm_cc));
                __builtin_memcpy (rp + sizeof (Tpm_rc) + sizeof (Tpm_cc), pa, ps);

                // Update Session from Auth
                auto const auth { std::start_lifetime_as<Tpms_auth_response<H> const> (pa + ps) };
                attr = auth->attr;

                // Temporary HMAC buffer
                uint8_t hmac[H::dig_size];

                // Compute HMAC (HASH (rp))
                compute_hmac (hmac, auth->nonce.dig, rp, sizeof (rp));

                // Verify response HMAC
                return !memcmp (hmac, auth->hmac.dig, sizeof (hmac));
            }
        };

        using Hmac_session = Session<Hash_sha2_256>;
};
