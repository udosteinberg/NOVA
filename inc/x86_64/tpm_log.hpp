/*
 * Trusted Platform Module (TPM) Crypto Agile Event Log
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

#include "tpm_alg.hpp"

class Tpm_log final
{
    friend class Hip_arch;

    private:
        static inline uint64_t  phys { 0 };
        static inline uint32_t  size { 0 };
        static inline uint32_t  offs { 0 };
        static inline size_t    tdsz { 0 };
        static inline Hash_bmp  hash;

        enum class Event : uint32_t
        {
            EV_NO_ACTION        = 0x3,
            EV_EVENT_TAG        = 0x6,
        };

        struct Header
        {
            uint32_t            pcr;                            //  0
            Event               evt;                            //  4
            uint8_t             dig[Hash_sha1_160::digest];     //  8
            uint32_t            esz;                            // 28
            uint32_t            sig[4];                         // 32
            uint32_t            cls;                            // 48
            uint8_t             min, maj, rev, usz;             // 52
            uint32_t            cnt;                            // 56
        };

        static_assert (__is_standard_layout (Header) && sizeof (Header) == 60);

        struct Algorithm
        {
            Tcg::Tpm_alg::Type  alg;                            //  0
            uint16_t            dsz;                            //  2
        };

        static_assert (__is_standard_layout (Algorithm) && sizeof (Algorithm) == 4);

        template <typename T>
        static void add_digest (Hash_bmp::Type h, Tcg::Tpm_alg::Type a, T const &d, uint8_t *&p)
        {
            // Ignore unsupported hash algorithms
            if (EXPECT_FALSE (!hash.supported (h)))
                return;

            // Add algorithm identifier
            le_uint16_t { std::to_underlying (a) }.serialize (p);

            // Add digest
            p = d.serialize (p + sizeof (le_uint16_t));
        }

    public:
        static void init (uint64_t, uint32_t, uint32_t);

        static bool extend (unsigned, Hash_sha1_160 const &, Hash_sha2_256 const &, Hash_sha2_384 const &, Hash_sha2_512 const &);
};
