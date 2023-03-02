/*
 * Trusted Computing Group (TCG) Definitions
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "hash.hpp"

class Tcg
{
    private:
        // Typed Size
        template<typename T> struct Size : private Unaligned_be<T>
        {
            auto size() const { return T { *this }; }

            explicit Size (T v) : Unaligned_be<T> { v } {}
        };

        // Typed List
        template<typename T> struct Tpml : public Size<uint32_t>
        {
            auto next() const { return reinterpret_cast<T const *>(this + 1); }

            explicit Tpml (uint32_t s) : Size { s } {}
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
         * TPM Specification Family 2.0 (Part 2), Table 9: Algorithm ID
         */
        struct Tpm_ai final : private Unaligned_be<uint16_t>
        {
            enum class Type : uint16_t
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

            auto type() const { return Type { uint16_t { *this } }; }

            explicit Tpm_ai (Type t) : Unaligned_be<uint16_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_ai) == 1 && sizeof (Tpm_ai) == sizeof (uint16_t));

    protected:
        /*
         * TPM Specification Family 2.0 (Part 2), Table 12: Command Code
         */
        struct Tpm_cc : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                V1_PCR_EXTEND               = 0x014,        // TPM 1.2
                V1_GET_CAPABILITY           = 0x065,        // TPM 1.2
                V1_SHUTDOWN                 = 0x098,        // TPM 1.2
                V1_PCR_RESET                = 0x0c8,        // TPM 1.2

                V2_PCR_RESET                = 0x13d,        // TPM 2.0
                V2_SHUTDOWN                 = 0x145,        // TPM 2.0
                V2_GET_CAPABILITY           = 0x17a,        // TPM 2.0
                V2_PCR_EXTEND               = 0x182,        // TPM 2.0
            };

            auto type() const { return Type { uint32_t { *this } }; }

            explicit Tpm_cc (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_cc) == 1 && sizeof (Tpm_cc) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 16: Response Code
         */
        struct Tpm_rc : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                RC_SUCCESS                  = 0x000,
            };

            auto type() const { return Type { uint32_t { *this } }; }
        };

        static_assert (alignof (Tpm_rc) == 1 && sizeof (Tpm_rc) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 19: Structure Tag
         */
        struct Tpm_st : private Unaligned_be<uint16_t>
        {
            enum class Type : uint16_t
            {
                RQU_COMMAND                 = 0x00c1,       // TPM 1.2
                RSP_COMMAND                 = 0x00c4,       // TPM 1.2
                ST_NO_SESSIONS              = 0x8001,       // TPM 2.0
                ST_SESSIONS                 = 0x8002,       // TPM 2.0
            };

            explicit Tpm_st (Type t) : Unaligned_be<uint16_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_st) == 1 && sizeof (Tpm_st) == sizeof (uint16_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 20: Startup Type
         */
        struct Tpm_su final : private Unaligned_be<uint16_t>
        {
            enum class Type : uint16_t
            {
                SU_CLEAR                    = 0x0000,
                SU_STATE                    = 0x0001,
            };

            explicit Tpm_su (Type t) : Unaligned_be<uint16_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm_su) == 1 && sizeof (Tpm_su) == sizeof (uint16_t));

        /*
         * TPM Specification Family 1.2 (Part 2), Section 21.1 (2285): Capability
         */
        struct Tpm1_cap final : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                CAP_PROPERTY                = 0x5,
            };

            auto type() const { return Type { uint32_t { *this } }; }

            explicit Tpm1_cap (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm1_cap) == 1 && sizeof (Tpm1_cap) == sizeof (uint32_t));

        /*
         * TPM Specification Family 1.2 (Part 2), Section 21.2 (2308): Property Tag
         */
        struct Tpm1_ptg final : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                PTG_PCR_COUNT               = 0x101,
                PTG_MANUFACTURER            = 0x103,
                PTG_INPUT_BUFFER            = 0x124,
            };

            auto type() const { return Type { uint32_t { *this } }; }

            explicit Tpm1_ptg (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm1_ptg) == 1 && sizeof (Tpm1_ptg) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 22: Capability
         */
        struct Tpm2_cap final : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                CAP_PCRS                    = 0x5,
                CAP_TPM_PROPERTIES          = 0x6,
            };

            auto type() const { return Type { uint32_t { *this } }; }

            explicit Tpm2_cap (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm2_cap) == 1 && sizeof (Tpm2_cap) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 23: Property Tag
         */
        struct Tpm2_ptg final : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                PTG_MANUFACTURER            = 0x105,
                PTG_INPUT_BUFFER            = 0x10d,
                PTG_PCR_COUNT               = 0x112,
            };

            auto type() const { return Type { uint32_t { *this } }; }

            explicit Tpm2_ptg (Type t) : Unaligned_be<uint32_t> { std::to_underlying (t) } {}
        };

        static_assert (alignof (Tpm2_ptg) == 1 && sizeof (Tpm2_ptg) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 26: TPM Handle
         */
        struct Tpm2_hdl final : private Unaligned_be<uint32_t>
        {
            enum class Type : uint32_t
            {
                HT_PCR                      = 0x00,
                HT_PERMANENT                = 0x40,
            };

            explicit Tpm2_hdl (Type t, uint32_t n) : Unaligned_be<uint32_t> { std::to_underlying (t) << 24 | n } {}
        };

        static_assert (alignof (Tpm2_hdl) == 1 && sizeof (Tpm2_hdl) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 32
         */
        struct Tpma_session final : private Unaligned_be<uint8_t>
        {
            explicit Tpma_session (uint8_t a) : Unaligned_be<uint8_t> { a } {}
        };

        static_assert (alignof (Tpma_session) == 1 && sizeof (Tpma_session) == sizeof (uint8_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 80
         */
        struct Tpm2b_digest final : public Size<uint16_t>                           // Size is flexible
        {
            auto dgst() const { return reinterpret_cast<uint8_t const *>(this + 1); }
            auto next() const { return reinterpret_cast<decltype (this)>(dgst() + size()); }

            explicit Tpm2b_digest (uint16_t s) : Size { s } {}
        };

        static_assert (alignof (Tpm2b_digest) == 1 && sizeof (Tpm2b_digest) == sizeof (uint16_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 92
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

            explicit Tpms_pcr_select (uint8_t s) : Size { s } {}
        };

        static_assert (alignof (Tpms_pcr_select) == 1 && sizeof (Tpms_pcr_select) == sizeof (uint8_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 93
         */
        struct Tpms_pcr_selection final                                             // Size is flexible because of Tpms_pcr_select
        {
            Tpm_ai                  const alg;                                      // Hash Algorithm
            Tpms_pcr_select         const sel;                                      // PCR Selection

            auto next() const { return reinterpret_cast<decltype (this)>(sel.next()); }

            explicit Tpms_pcr_selection (Tpm_ai::Type a, uint8_t s) : alg { a }, sel { s } {}
        };

        static_assert (alignof (Tpms_pcr_selection) == 1 && sizeof (Tpms_pcr_selection) == sizeof (Tpm_ai) + sizeof (Tpms_pcr_select));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 101
         */
        struct Tpms_tagged_property final
        {
            Tpm2_ptg                const ptg;                                      // Property Tag
            Unaligned_be<uint32_t>  const val;                                      // Property Value

            auto next() const { return this + 1; }
        };

        static_assert (alignof (Tpms_tagged_property) == 1 && sizeof (Tpms_tagged_property) == sizeof (Tpm2_ptg) + sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 109
         */
        struct Tpml_digest final : public Tpml<Tpm2b_digest> {};

        static_assert (alignof (Tpml_digest) == 1 && sizeof (Tpml_digest) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 110
         */
        struct Tpml_digest_values final : public Tpml<Tpm_ai>
        {
            explicit Tpml_digest_values (uint32_t s) : Tpml { s } {}
        };

        static_assert (alignof (Tpml_digest_values) == 1 && sizeof (Tpml_digest_values) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 111
         */
        struct Tpml_pcr_selection final : public Tpml<Tpms_pcr_selection>
        {
            explicit Tpml_pcr_selection (uint32_t s) : Tpml { s } {}
        };

        static_assert (alignof (Tpml_pcr_selection) == 1 && sizeof (Tpml_pcr_selection) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 113
         */
        struct Tpml_tagged_tpm_property final : public Tpml<Tpms_tagged_property> {};

        static_assert (alignof (Tpml_tagged_tpm_property) == 1 && sizeof (Tpml_tagged_tpm_property) == sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 2), Table 119
         */
        struct Tpms_capability_data final
        {
            Tpm2_cap                const cap;

            template<typename T> auto next() const { return reinterpret_cast<T const *>(this + 1); }
        };

        static_assert (alignof (Tpms_capability_data) && sizeof (Tpms_capability_data) == sizeof (Tpm2_cap));

        /*
         * TPM Specification Family 2.0 (Part 1), Table 12
         */
        struct Empty_auth final
        {
            Tpm2_hdl                const auth      { Tpm2_hdl { Tpm2_hdl::Type::HT_PERMANENT, 9 } };
            Tpm2b_digest            const nonce     { 0 };
            Tpma_session            const attr      { 0 };
            Tpm2b_digest            const pass      { 0 };
        };

        static_assert (alignof (Empty_auth) == 1 && sizeof (Empty_auth) == sizeof (Tpm2_hdl) + 2 * sizeof (Tpm2b_digest) + sizeof (Tpma_session));

        /*
         * TPM Specification Family 2.0 (Part 1), Section 18.2
         */
        struct Hdr : protected Tpm_st, public Size<uint32_t>
        {
            explicit Hdr (Tpm_st::Type t, uint32_t s) : Tpm_st { t }, Size { s } {}
        };

        static_assert (alignof (Hdr) == 1 && sizeof (Hdr) == sizeof (Tpm_st) + sizeof (uint32_t));

        /*
         * TPM Specification Family 2.0 (Part 1), Section 18.2
         */
        struct Cmd : public Hdr, public Tpm_cc
        {
            explicit Cmd (Tpm_st::Type t, uint32_t s, Tpm_cc::Type c) : Hdr { t, s }, Tpm_cc { c } {}
        };

        static_assert (alignof (Cmd) == 1 && sizeof (Cmd) == sizeof (Hdr) + sizeof (Tpm_cc));

        /*
         * TPM Specification Family 2.0 (Part 1), Section 18.2
         */
        struct Res final : public Hdr, public Tpm_rc {};

        static_assert (alignof (Res) == 1 && sizeof (Res) == sizeof (Hdr) + sizeof (Tpm_rc));

        /*
         * TPM Specification Family 1.2 (Part 3), Section 3.3 (430)
         */
        class Tpm1_shutdown final : public Cmd
        {
            public:
                explicit Tpm1_shutdown() : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_SHUTDOWN } {}
        };

        static_assert (alignof (Tpm1_shutdown) == 1 && sizeof (Tpm1_shutdown) == sizeof (Cmd));

        /*
         * TPM Specification Family 1.2 (Part 3), Section 7.1 (921)
         */
        class Tpm1_get_capability final : public Cmd
        {
            private:
                Tpm1_cap                  const cap     { Tpm1_cap::Type::CAP_PROPERTY };           // Capability Area
                Unaligned_be<uint32_t>    const size    { sizeof (ptg) };                           // Subcap Size
                Tpm1_ptg                  const ptg;                                                // Property Tag

            public:
                explicit Tpm1_get_capability (Tpm1_ptg::Type p) : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_GET_CAPABILITY }, ptg { p } {}
        };

        static_assert (alignof (Tpm1_get_capability) == 1 && sizeof (Tpm1_get_capability) == sizeof (Cmd) + sizeof (Tpm1_cap) + sizeof (uint32_t) + sizeof (Tpm1_ptg));

        /*
         * TPM Specification Family 1.2 (Part 3), Section 16.1 (3050)
         */
        class Tpm1_pcr_extend final : public Cmd
        {
            private:
                Unaligned_be<uint32_t>    const pidx;                                               // PCR Index
                uint8_t                         sha1_160[Hash_sha1_160::digest];                    // Hash Digest

            public:
                explicit Tpm1_pcr_extend (unsigned pcr, Hash_sha1_160 const &h) : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_PCR_EXTEND }, pidx { pcr } { h.serialize (sha1_160); }
        };

        static_assert (alignof (Tpm1_pcr_extend) == 1 && sizeof (Tpm1_pcr_extend) == sizeof (Cmd) + sizeof (uint32_t) + Hash_sha1_160::digest);

        /*
         * TPM Specification Family 1.2 (Part 3), Section 16.4 (3149)
         */
        class Tpm1_pcr_reset final : public Cmd
        {
            private:
                Unaligned_be<uint16_t>    const size    { sizeof (psel) };                          // PCR Selection Size
                Pcr_bitmap<3>             const psel;                                               // PCR Selection

            public:
                explicit Tpm1_pcr_reset (unsigned pcr) : Cmd { Tpm_st::Type::RQU_COMMAND, sizeof (*this), Tpm_cc::Type::V1_PCR_RESET }, psel { BIT64 (pcr) } {}
        };

        static_assert (alignof (Tpm1_pcr_reset) == 1 && sizeof (Tpm1_pcr_reset) == sizeof (Cmd) + sizeof (uint16_t) + sizeof (Pcr_bitmap<3>));

        /*
         * TPM Specification Family 2.0 (Part 3), Table 7
         */
        class Tpm2_shutdown final : public Cmd
        {
            private:
                Tpm_su const su;

            public:
                explicit Tpm2_shutdown (Tpm_su::Type t) : Cmd { Tpm_st::Type::ST_NO_SESSIONS, sizeof (*this), Tpm_cc::Type::V2_SHUTDOWN }, su { t } {}
        };

        static_assert (alignof (Tpm2_shutdown) == 1 && sizeof (Tpm2_shutdown) == sizeof (Cmd) + sizeof (Tpm_su));

        /*
         * TPM Specification Family 2.0 (Part 3), Table 110
         */
        class Tpm2_pcr_extend final : public Cmd
        {
            private:
                Tpm2_hdl                  const hpcr;                                               // Handle: PCR
                Unaligned_be<uint32_t>    const size    { sizeof (auth) };                          // Authorization: Size
                Empty_auth                const auth;                                               // Authorization: PCR
                Tpml_digest_values        const tpml    { 1 };                                      // Number of Digests
                Tpm_ai                    const halg;                                               // Hash Algorithm
                union {                                                                             // Hash Digest
                    uint8_t                     sha1_160[Hash_sha1_160::digest];
                    uint8_t                     sha2_256[Hash_sha2_256::digest];
                    uint8_t                     sha2_384[Hash_sha2_384::digest];
                    uint8_t                     sha2_512[Hash_sha2_512::digest];
                } digest;

            public:
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha1_160 const &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha1_160), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm2_hdl { Tpm2_hdl::Type::HT_PCR, pcr } }, halg { Tpm_ai::Type::SHA1_160 } { h.serialize (digest.sha1_160); }
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha2_256 const &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_256), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm2_hdl { Tpm2_hdl::Type::HT_PCR, pcr } }, halg { Tpm_ai::Type::SHA2_256 } { h.serialize (digest.sha2_256); }
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha2_384 const &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_384), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm2_hdl { Tpm2_hdl::Type::HT_PCR, pcr } }, halg { Tpm_ai::Type::SHA2_384 } { h.serialize (digest.sha2_384); }
                explicit Tpm2_pcr_extend (unsigned pcr, Hash_sha2_512 const &h) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this) - sizeof (digest) + sizeof (digest.sha2_512), Tpm_cc::Type::V2_PCR_EXTEND }, hpcr { Tpm2_hdl { Tpm2_hdl::Type::HT_PCR, pcr } }, halg { Tpm_ai::Type::SHA2_512 } { h.serialize (digest.sha2_512); }
        };

        static_assert (alignof (Tpm2_pcr_extend) == 1 && sizeof (Tpm2_pcr_extend) == sizeof (Cmd) + sizeof (Tpm2_hdl) + sizeof (uint32_t) + sizeof (Empty_auth) + sizeof (Tpml_digest_values) + sizeof (Tpm_ai) + Hash_sha2_512::digest);

        /*
         * TPM Specification Family 2.0 (Part 3), Table 122
         */
        class Tpm2_pcr_reset final : public Cmd
        {
            private:
                Tpm2_hdl                  const hpcr;                                               // Handle: PCR
                Unaligned_be<uint32_t>    const size    { sizeof (auth) };                          // Authorization: Size
                Empty_auth                const auth;                                               // Authorization: PCR

            public:
                explicit Tpm2_pcr_reset (unsigned pcr) : Cmd { Tpm_st::Type::ST_SESSIONS, sizeof (*this), Tpm_cc::Type::V2_PCR_RESET }, hpcr { Tpm2_hdl { Tpm2_hdl::Type::HT_PCR, pcr } } {}
        };

        static_assert (alignof (Tpm2_pcr_reset) == 1 && sizeof (Tpm2_pcr_reset) == sizeof (Cmd) + sizeof (Tpm2_hdl) + sizeof (uint32_t) + sizeof (Empty_auth));

        /*
         * TPM Specification Family 2.0 (Part 3), Table 208
         */
        class Tpm2_get_capability final : public Cmd
        {
            private:
                Tpm2_cap                  const cap;                                                // Capability
                Tpm2_ptg                  const ptg;                                                // Property Tag
                Unaligned_be<uint32_t>    const cnt;                                                // Number of Properties

            public:
                explicit Tpm2_get_capability (Tpm2_cap::Type c, Tpm2_ptg::Type p, uint32_t n) : Cmd { Tpm_st::Type::ST_NO_SESSIONS, sizeof (*this), Tpm_cc::Type::V2_GET_CAPABILITY }, cap { c }, ptg { p }, cnt { n } {}
        };

        static_assert (alignof (Tpm2_get_capability) == 1 && sizeof (Tpm2_get_capability) == sizeof (Cmd) + sizeof (Tpm2_cap) + sizeof (Tpm2_ptg) + sizeof (uint32_t));
};
