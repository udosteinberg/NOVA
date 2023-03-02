/*
 * Trusted Computing Group (TCG) Definitions
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

#include "byteorder.hpp"
#include "hash.hpp"
#include "std.hpp"

#pragma pack(1)

class Tcg
{
    private:
        // Typed Size
        template <typename T> struct Size : private Byteorder_be<T>
        {
            auto size() const { return static_cast<T>(*this); }

            explicit constexpr Size (T v) : Byteorder_be<T> { v } {}
        };

        // Typed List
        template <typename T> struct Tpml : public Size<uint32_t>
        {
            auto next() const { return reinterpret_cast<T const *>(this + 1); }

            explicit constexpr Tpml (uint32_t s) : Size { s } {}
        };

        // PCR Bitmap
        template <unsigned N> struct Pcr_bitmap
        {
            uint8_t pcrs[N];

            explicit constexpr Pcr_bitmap (uint64_t v)
            {
                for (size_t i { 0 }; i < sizeof (pcrs); v >>= 8)
                    pcrs[i++] = static_cast<uint8_t>(v);
            }
        };

    protected:
        // Supported Hash Algorithms
        static inline uint32_t hash_algorithms { 0 };

        // TCG Algorithm Registry, 6.10: Hash Algorithms Bit Field
        enum Hash_alg : decltype (hash_algorithms)
        {
            SHA1_160    = BIT (0),
            SHA2_256    = BIT (1),
            SHA2_384    = BIT (2),
            SHA2_512    = BIT (3),
            SM3_256     = BIT (4),
            SHA3_256    = BIT (5),
            SHA3_384    = BIT (6),
            SHA3_512    = BIT (7),
        };

        static bool supports (Hash_alg a) { return hash_algorithms & a; }

        // TPM Specification Family 2.0 (Part 2), Table 9: Algorithm ID
        struct Tpm_alg : private be_uint16_t
        {
            enum class Type : uint16_t
            {
                ALG_SHA1_160                = 0x0004,
                ALG_SHA2_256                = 0x000b,
                ALG_SHA2_384                = 0x000c,
                ALG_SHA2_512                = 0x000d,
                ALG_SM3_256                 = 0x0012,
                ALG_SHA3_256                = 0x0027,
                ALG_SHA3_384                = 0x0028,
                ALG_SHA3_512                = 0x0029,
            };

            auto type() const { return Type { static_cast<uint16_t>(*this) }; }

            explicit constexpr Tpm_alg (Type t) : be_uint16_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm_alg) == sizeof (uint16_t));

        // TPM Specification Family 2.0 (Part 2), Table 12: Command Code
        struct Tpm_cc : private be_uint32_t
        {
            enum class Type : uint32_t
            {
                V1_PCR_EXTEND               = 0x014,        // TPM 1.2
                V1_PCR_READ                 = 0x015,        // TPM 1.2
                V1_GET_CAPABILITY           = 0x065,        // TPM 1.2
                V1_SHUTDOWN                 = 0x098,        // TPM 1.2
                V1_PCR_RESET                = 0x0c8,        // TPM 1.2

                V2_PCR_RESET                = 0x13d,        // TPM 2.0
                V2_SHUTDOWN                 = 0x145,        // TPM 2.0
                V2_GET_CAPABILITY           = 0x17a,        // TPM 2.0
                V2_PCR_READ                 = 0x17e,        // TPM 2.0
                V2_PCR_EXTEND               = 0x182,        // TPM 2.0
            };

            auto type() const { return Type { static_cast<uint32_t>(*this) }; }

            explicit constexpr Tpm_cc (Type t) : be_uint32_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm_cc) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 16: Response Code
        struct Tpm_rc : private be_uint32_t
        {
            enum class Type : uint32_t
            {
                RC_SUCCESS                  = 0x000,
            };

            auto type() const { return Type { static_cast<uint32_t>(*this) }; }

            explicit constexpr Tpm_rc (Type t) : be_uint32_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm_rc) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 19: Structure Tag
        struct Tpm_st : private be_uint16_t
        {
            enum class Type : uint16_t
            {
                RQU_COMMAND                 = 0x00c1,       // TPM 1.2
                RSP_COMMAND                 = 0x00c4,       // TPM 1.2
                ST_NO_SESSIONS              = 0x8001,       // TPM 2.0
                ST_SESSIONS                 = 0x8002,       // TPM 2.0
            };

            explicit constexpr Tpm_st (Type t) : be_uint16_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm_st) == sizeof (uint16_t));

        // TPM Specification Family 2.0 (Part 2), Table 20: Startup Type
        struct Tpm_su : private be_uint16_t
        {
            enum class Type : uint16_t
            {
                SU_CLEAR                    = 0x0000,
                SU_STATE                    = 0x0001,
            };

            explicit constexpr Tpm_su (Type t) : be_uint16_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm_su) == sizeof (uint16_t));

        // TPM Specification Family 1.2 (Part 2), Section 21.1 (2285): Capability
        struct Tpm12_cap : private be_uint32_t
        {
            enum class Type : uint32_t
            {
                CAP_PROPERTY                = 0x5,
            };

            auto type() const { return Type { static_cast<uint32_t>(*this) }; }

            explicit constexpr Tpm12_cap (Type t) : be_uint32_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm12_cap) == sizeof (uint32_t));

        // TPM Specification Family 1.2 (Part 2), Section 21.2 (2308): Property Tag
        struct Tpm12_ptg : private be_uint32_t
        {
            enum class Type : uint32_t
            {
                PTG_PCR_COUNT               = 0x101,
                PTG_MANUFACTURER            = 0x103,
                PTG_INPUT_BUFFER            = 0x124,
            };

            auto type() const { return Type { static_cast<uint32_t>(*this) }; }

            explicit constexpr Tpm12_ptg (Type t) : be_uint32_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm12_ptg) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 22: Capability
        struct Tpm20_cap : private be_uint32_t
        {
            enum class Type : uint32_t
            {
                CAP_ALGS                    = 0x0,
                CAP_HANDLES                 = 0x1,
                CAP_COMMANDS                = 0x2,
                CAP_PP_COMMANDS             = 0x3,
                CAP_AUDIT_COMMANDS          = 0x4,
                CAP_PCRS                    = 0x5,
                CAP_TPM_PROPERTIES          = 0x6,
                CAP_PCR_PROPERTIES          = 0x7,
                CAP_ECC_CURVES              = 0x8,
                CAP_AUTH_POLICIES           = 0x9,
                CAP_ACT                     = 0xa,
            };

            auto type() const { return Type { static_cast<uint32_t>(*this) }; }

            explicit constexpr Tpm20_cap (Type t) : be_uint32_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm20_cap) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 23: Property Tag
        struct Tpm20_ptg : private be_uint32_t
        {
            enum class Type : uint32_t
            {
                PTG_MANUFACTURER            = 0x105,
                PTG_INPUT_BUFFER            = 0x10d,
                PTG_PCR_COUNT               = 0x112,
                PTG_MAX_DIGEST              = 0x120,
            };

            auto type() const { return Type { static_cast<uint32_t>(*this) }; }

            explicit constexpr Tpm20_ptg (Type t) : be_uint32_t { std::to_underlying (t) } {}
        };

        static_assert (sizeof (Tpm20_ptg) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 26
        struct Tpm_handle : private be_uint32_t
        {
            // TPM Specification Family 2.0 (Part 2), Table 27
            enum class Type : uint32_t
            {
                HT_PCR                      = 0x00,
                HT_PERMANENT                = 0x40,
            };

            explicit constexpr Tpm_handle (Type t, uint32_t n) : be_uint32_t { std::to_underlying (t) << 24 | n } {}
        };

        static_assert (sizeof (Tpm_handle) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 32
        struct Tpma_session : private be_uint8_t
        {
            explicit constexpr Tpma_session (uint8_t a) : be_uint8_t { a } {}
        };

        static_assert (sizeof (Tpma_session) == sizeof (uint8_t));

        // TPM Specification Family 2.0 (Part 2), Table 79
        struct Tpmt_ha : private Tpm_alg
        {
            explicit constexpr Tpmt_ha (Tpm_alg::Type t) : Tpm_alg { t } {}
        };

        static_assert (sizeof (Tpmt_ha) == sizeof (Tpm_alg));

        // TPM Specification Family 2.0 (Part 2), Table 80
        struct Tpm2b_digest : public Size<uint16_t>
        {
            // Tpm2b_digest size is flexible
            auto dgst() const { return reinterpret_cast<uint8_t const *>(this + 1); }
            auto next() const { return reinterpret_cast<decltype (this)>(reinterpret_cast<uintptr_t>(this + 1) + size()); }

            explicit constexpr Tpm2b_digest (uint16_t s) : Size { s } {}
        };

        static_assert (sizeof (Tpm2b_digest) == sizeof (uint16_t));

        // TPM Specification Family 2.0 (Part 2), Table 92
        struct Tpms_pcr_select : private Size<uint8_t>
        {
            auto pcrs() const
            {
                auto const b { reinterpret_cast<uint8_t const *>(this + 1) };       // PCR Select Bitmap

                uint64_t v { 0 };
                for (auto i { size() }; i--; v = v << 8 | b[i]) ;
                return v;
            }

            // Tpms_pcr_select size is flexible
            auto next() const { return reinterpret_cast<uintptr_t>(this + 1) + size(); }

            explicit constexpr Tpms_pcr_select (uint8_t s) : Size { s } {}
        };

        static_assert (sizeof (Tpms_pcr_select) == sizeof (uint8_t));

        // TPM Specification Family 2.0 (Part 2), Table 93
        struct Tpms_pcr_selection
        {
            Tpm_alg                 const alg;                                      // Hash Algorithm
            Tpms_pcr_select         const sel;                                      // PCR Selection

            // Tpms_pcr_selection size is flexible because of Tpms_pcr_select
            auto next() const { return reinterpret_cast<decltype (this)>(sel.next()); }

            explicit constexpr Tpms_pcr_selection (Tpm_alg::Type a, uint8_t s) : alg { a }, sel { s } {}
        };

        static_assert (sizeof (Tpms_pcr_selection) == sizeof (Tpm_alg) + sizeof (Tpms_pcr_select));

        // TPM Specification Family 2.0 (Part 2), Table 101
        struct Tpms_tagged_property
        {
            Tpm20_ptg               const ptg;                                      // Property Tag
            be_uint32_t             const val;                                      // Property Value

            auto next() const { return this + 1; }
        };

        static_assert (sizeof (Tpms_tagged_property) == sizeof (Tpm20_ptg) + sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 109
        struct Tpml_digest : public Tpml<Tpm2b_digest> {};

        static_assert (sizeof (Tpml_digest) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 110
        struct Tpml_digest_values : public Tpml<Tpmt_ha>
        {
            explicit constexpr Tpml_digest_values (uint32_t s) : Tpml { s } {}
        };

        static_assert (sizeof (Tpml_digest_values) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 111
        struct Tpml_pcr_selection : public Tpml<Tpms_pcr_selection>
        {
            explicit constexpr Tpml_pcr_selection (uint32_t s) : Tpml { s } {}
        };

        static_assert (sizeof (Tpml_pcr_selection) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 113
        struct Tpml_tagged_tpm_property : public Tpml<Tpms_tagged_property> {};

        static_assert (sizeof (Tpml_tagged_tpm_property) == sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 2), Table 119
        struct Tpms_capability_data : public Tpm20_cap
        {
            template<typename T> auto next() const { return reinterpret_cast<T const *>(this + 1); }
        };

        static_assert (sizeof (Tpms_capability_data) == sizeof (Tpm20_cap));

        // TPM Specification Family 2.0 (Part 1), Table 12
        struct Empty_auth
        {
            Tpm_handle              const auth      { Tpm_handle { Tpm_handle::Type::HT_PERMANENT, 9 } };
            Tpm2b_digest            const nonce     { 0 };
            Tpma_session            const attr      { 0 };
            Tpm2b_digest            const pass      { 0 };
        };

        static_assert (sizeof (Empty_auth) == sizeof (Tpm_handle) + 2 * sizeof (Tpm2b_digest) + sizeof (Tpma_session));

        // TPM Specification Family 2.0 (Part 1), Section 18.2
        struct Hdr : protected Tpm_st, public Size<uint32_t>
        {
            explicit constexpr Hdr (Tpm_st::Type t, uint32_t s) : Tpm_st { t }, Size { s } {}
        };

        static_assert (sizeof (Hdr) == sizeof (Tpm_st) + sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 1), Section 18.2
        struct Cmd : public Hdr, public Tpm_cc
        {
            explicit constexpr Cmd (Tpm_st::Type t, uint32_t s, Tpm_cc::Type c) : Hdr { t, s }, Tpm_cc { c } {}
        };

        static_assert (sizeof (Cmd) == sizeof (Hdr) + sizeof (Tpm_cc));

        // TPM Specification Family 2.0 (Part 1), Section 18.2
        struct Res : public Hdr, public Tpm_rc {};

        static_assert (sizeof (Res) == sizeof (Hdr) + sizeof (Tpm_rc));

        // TPM Specification Family 1.2 (Part 3), Section 3.3 (430)
        struct Tpm12_shutdown : public Cmd
        {
            explicit Tpm12_shutdown() : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_SHUTDOWN } {}
        };

        static_assert (sizeof (Tpm12_shutdown) == sizeof (Cmd));

        // TPM Specification Family 1.2 (Part 3), Section 7.1 (921)
        struct Tpm12_get_capability : public Cmd
        {
            Tpm12_cap             const cap     { Tpm12_cap::Type::CAP_PROPERTY };              // Capability Area
            be_uint32_t           const size    { sizeof (ptg) };                               // Subcap Size
            Tpm12_ptg             const ptg;                                                    // Property Tag

            explicit Tpm12_get_capability (Tpm12_ptg::Type p) : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_GET_CAPABILITY }, ptg { p } {}
        };

        static_assert (sizeof (Tpm12_get_capability) == sizeof (Cmd) + sizeof (Tpm12_cap) + sizeof (uint32_t) + sizeof (Tpm12_ptg));

        // TPM Specification Family 1.2 (Part 3), Section 7.1 (922)
        struct Tpm12_get_capability_res : public Res
        {
            be_uint32_t           const len;
        };

        static_assert (sizeof (Tpm12_get_capability_res) == sizeof (Res) + sizeof (uint32_t));

        // TPM Specification Family 1.2 (Part 3), Section 16.1 (3050)
        struct Tpm12_pcr_extend : public Cmd
        {
            be_uint32_t           const pidx;                                                   // PCR Index
            uint8_t                     sha1_160[20];                                           // Hash Digest

            explicit Tpm12_pcr_extend (unsigned pcr, Hash_sha1_160 &h) : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_PCR_EXTEND }, pidx { pcr } { h.digest (sha1_160); }
        };

        static_assert (sizeof (Tpm12_pcr_extend) == sizeof (Cmd) + sizeof (uint32_t) + 20);

        // TPM Specification Family 1.2 (Part 3), Section 16.2 (3074)
        struct Tpm12_pcr_read : public Cmd
        {
            be_uint32_t           const pidx;                                                   // PCR Index

            explicit Tpm12_pcr_read (unsigned pcr) : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_PCR_READ }, pidx { pcr } {}
        };

        static_assert (sizeof (Tpm12_pcr_read) == sizeof (Cmd) + sizeof (uint32_t));

        // TPM Specification Family 1.2 (Part 3), Section 16.2 (3075)
        struct Tpm12_pcr_read_res : public Res
        {
            uint8_t               const sha1_160[20];                                           // Hash Digest
        };

        static_assert (sizeof (Tpm12_pcr_read_res) == sizeof (Res) + 20);

        // TPM Specification Family 1.2 (Part 3), Section 16.4 (3149)
        struct Tpm12_pcr_reset : public Cmd
        {
            be_uint16_t           const size    { sizeof (psel) };                              // PCR Selection Size
            Pcr_bitmap<3>         const psel;                                                   // PCR Selection

            explicit Tpm12_pcr_reset (unsigned pcr) : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_PCR_RESET }, psel { BIT64 (pcr) } {}
        };

        static_assert (sizeof (Tpm12_pcr_reset) == sizeof (Cmd) + sizeof (uint16_t) + sizeof (Pcr_bitmap<3>));

        // TPM Specification Family 2.0 (Part 3), Table 7
        struct Tpm20_shutdown : public Cmd, private Tpm_su
        {
            explicit Tpm20_shutdown (Tpm_su::Type t) : Cmd { Tpm_st::Type::ST_NO_SESSIONS, sizeof (*this), Tpm_cc::Type::V2_SHUTDOWN }, Tpm_su { t } {}
        };

        static_assert (sizeof (Tpm20_shutdown) == sizeof (Cmd) + sizeof (Tpm_su));

        // TPM Specification Family 2.0 (Part 3), Table 110
        struct Tpm20_pcr_extend : public Cmd
        {
            Tpm_handle            const hpcr;                                                   // Handle: PCR
            be_uint32_t           const size    { sizeof (auth) };                              // Authorization: Size
            Empty_auth            const auth;                                                   // Authorization: PCR
            Tpml_digest_values    const list    { 1 };                                          // Number of Digests
            Tpmt_ha               const halg;                                                   // Hash Algorithm
            union {                                                                             // Hash Digest
                uint8_t                 sha1_160[20];
                uint8_t                 sha2_256[32];
                uint8_t                 sha2_384[48];
                uint8_t                 sha2_512[64];
            } digest;

            explicit Tpm20_pcr_extend (unsigned pcr, Hash_sha1_160 &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha1_160), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm_handle { Tpm_handle::Type::HT_PCR, pcr } }, halg { Tpm_alg::Type::ALG_SHA1_160 } { h.digest (digest.sha1_160); }
            explicit Tpm20_pcr_extend (unsigned pcr, Hash_sha2_256 &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_256), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm_handle { Tpm_handle::Type::HT_PCR, pcr } }, halg { Tpm_alg::Type::ALG_SHA2_256 } { h.digest (digest.sha2_256); }
            explicit Tpm20_pcr_extend (unsigned pcr, Hash_sha2_384 &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_384), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm_handle { Tpm_handle::Type::HT_PCR, pcr } }, halg { Tpm_alg::Type::ALG_SHA2_384 } { h.digest (digest.sha2_384); }
            explicit Tpm20_pcr_extend (unsigned pcr, Hash_sha2_512 &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_512), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm_handle { Tpm_handle::Type::HT_PCR, pcr } }, halg { Tpm_alg::Type::ALG_SHA2_512 } { h.digest (digest.sha2_512); }
        };

        static_assert (sizeof (Tpm20_pcr_extend) == sizeof (Cmd) + sizeof (Tpm_handle) + sizeof (uint32_t) + sizeof (Empty_auth) + sizeof (Tpml_digest_values) + sizeof (Tpmt_ha) + 64);

        // TPM Specification Family 2.0 (Part 3), Table 114
        struct Tpm20_pcr_read : public Cmd
        {
            Tpml_pcr_selection    const list    { 2 };                                          // Number of Tpms_pcr_selection
            Tpms_pcr_selection    const alg1    { Tpm_alg::Type::ALG_SHA1_160, sizeof (sel1) }; // Hash Algorithm
            Pcr_bitmap<3>         const sel1;                                                   // PCR Selection
            Tpms_pcr_selection    const alg2    { Tpm_alg::Type::ALG_SHA2_256, sizeof (sel2) }; // Hash Algorithm
            Pcr_bitmap<3>         const sel2;                                                   // PCR Selection

            explicit Tpm20_pcr_read (unsigned pcr) : Cmd { Tpm_st::Type::ST_NO_SESSIONS, sizeof (*this), Tpm_cc::Type::V2_PCR_READ }, sel1 { BIT64 (pcr) }, sel2 { BIT64 (pcr) } {}
        };

        static_assert (sizeof (Tpm20_pcr_read) == sizeof (Cmd) + sizeof (Tpml_pcr_selection) + 2 * (sizeof (Tpms_pcr_selection) + sizeof (Pcr_bitmap<3>)));

        // TPM Specification Family 2.0 (Part 3), Table 115
        struct Tpm20_pcr_read_res : public Res
        {
            be_uint32_t           const ucnt;                                                   // PCR Update Counter
        };

        static_assert (sizeof (Tpm20_pcr_read_res) == sizeof (Res) + sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 3), Table 122
        struct Tpm20_pcr_reset : public Cmd
        {
            Tpm_handle            const hpcr;                                                   // Handle: PCR
            be_uint32_t           const size    { sizeof (auth) };                              // Authorization: Size
            Empty_auth            const auth;                                                   // Authorization: PCR

            explicit Tpm20_pcr_reset (unsigned pcr) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this), Tpm_cc::Type::V2_PCR_RESET }, hpcr { Tpm_handle { Tpm_handle::Type::HT_PCR, pcr } } {}
        };

        static_assert (sizeof (Tpm20_pcr_reset) == sizeof (Cmd) + sizeof (Tpm_handle) + sizeof (uint32_t) + sizeof (Empty_auth));

        // TPM Specification Family 2.0 (Part 3), Table 208
        struct Tpm20_get_capability : public Cmd
        {
            Tpm20_cap             const cap;                                                    // Capability
            Tpm20_ptg             const ptg;                                                    // Property Tag
            be_uint32_t           const cnt;                                                    // Number of Properties

            explicit Tpm20_get_capability (Tpm20_cap::Type c, Tpm20_ptg::Type p, uint32_t n) : Cmd { Tpm_st::Type::ST_NO_SESSIONS, sizeof (*this), Tpm_cc::Type::V2_GET_CAPABILITY }, cap { c }, ptg { p }, cnt { n } {}
        };

        static_assert (sizeof (Tpm20_get_capability) == sizeof (Cmd) + sizeof (Tpm20_cap) + sizeof (Tpm20_ptg) + sizeof (uint32_t));

        // TPM Specification Family 2.0 (Part 3), Table 209
        struct Tpm20_get_capability_res : public Res
        {
            bool                  const more;                                                   // More Data
            Tpms_capability_data  const data;                                                   // Capability Data
        };

        static_assert (sizeof (Tpm20_get_capability_res) == sizeof (Res) + sizeof (bool) + sizeof (Tpms_capability_data));

        // Digest Render Helper
        static void digest_render (char *b, uint8_t const *d, unsigned l)
        {
            for (static constexpr auto s { "0123456789abcdef" }; l--; d++) {
                *b++ = s[BIT_RANGE (3, 0) & *d >> 4];
                *b++ = s[BIT_RANGE (3, 0) & *d];
            }

            *b = 0;
        }
};

#pragma pack()
