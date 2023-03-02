/*
 * Trusted Platform Module (TPM) Hash Algorithms
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

#include "tcg.hpp"

/*
 * TCG Algorithm Registry v2.0, Table 7: Hash Algorithms Bit Field
 */
class Hash_bmp final
{
    private:
        uint32_t bmp {};

    public:
        enum Type : uint32_t
        {
            SHA1_160    = BIT (0),      // Deprecated
            SHA2_256    = BIT (1),      // Mandatory
            SHA2_384    = BIT (2),      // Mandatory
            SHA2_512    = BIT (3),      // Mandatory
            SM3_256     = BIT (4),      // Optional
            SHA3_256    = BIT (5),      // Optional
            SHA3_384    = BIT (6),      // Optional
            SHA3_512    = BIT (7),      // Optional
        };

        static constexpr unsigned dig_size (Tcg::Tpm_alg_id::Hash h)
        {
            switch (h) {
                case Tcg::Tpm_alg_id::Hash::SHA1_160: return Hash_sha1_160::dig_size;
                case Tcg::Tpm_alg_id::Hash::SHA2_256: return Hash_sha2_256::dig_size;
                case Tcg::Tpm_alg_id::Hash::SHA2_384: return Hash_sha2_384::dig_size;
                case Tcg::Tpm_alg_id::Hash::SHA2_512: return Hash_sha2_512::dig_size;
                case Tcg::Tpm_alg_id::Hash::SM3_256:  return 32;
                case Tcg::Tpm_alg_id::Hash::SHA3_256: return Hash_sha3_256::dig_size;
                case Tcg::Tpm_alg_id::Hash::SHA3_384: return Hash_sha3_384::dig_size;
                case Tcg::Tpm_alg_id::Hash::SHA3_512: return Hash_sha3_512::dig_size;
                default:                              return 0;
            }
        }

        bool add (Tcg::Tpm_alg_id::Hash h)
        {
            uint32_t o { bmp }, b { 0 };

            switch (h) {
                case Tcg::Tpm_alg_id::Hash::SHA1_160: b = Type::SHA1_160; break;
                case Tcg::Tpm_alg_id::Hash::SHA2_256: b = Type::SHA2_256; break;
                case Tcg::Tpm_alg_id::Hash::SHA2_384: b = Type::SHA2_384; break;
                case Tcg::Tpm_alg_id::Hash::SHA2_512: b = Type::SHA2_512; break;
                case Tcg::Tpm_alg_id::Hash::SM3_256:  b = Type::SM3_256;  break;
                case Tcg::Tpm_alg_id::Hash::SHA3_256: b = Type::SHA3_256; break;
                case Tcg::Tpm_alg_id::Hash::SHA3_384: b = Type::SHA3_384; break;
                case Tcg::Tpm_alg_id::Hash::SHA3_512: b = Type::SHA3_512; break;
            }

            return (bmp |= b) != o;
        }

        constexpr auto count() const { return static_cast<unsigned>(__builtin_popcount (bmp)); }

        constexpr bool supported (Type t) const { return bmp & t; }
};
