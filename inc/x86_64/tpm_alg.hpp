/*
 * Trusted Platform Module (TPM) Hash Algorithms
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

#include "tcg.hpp"

/*
 * TCG Algorithm Registry, 6.10: Hash Algorithms Bit Field
 */
class Hash_bmp final
{
    private:
        uint32_t bmp { 0 };

    public:
        enum Type : uint32_t
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

        void add (Tcg::Tpm_alg::Type t)
        {
            switch (t) {
                case Tcg::Tpm_alg::Type::SHA1_160: bmp |= Type::SHA1_160; break;
                case Tcg::Tpm_alg::Type::SHA2_256: bmp |= Type::SHA2_256; break;
                case Tcg::Tpm_alg::Type::SHA2_384: bmp |= Type::SHA2_384; break;
                case Tcg::Tpm_alg::Type::SHA2_512: bmp |= Type::SHA2_512; break;
                case Tcg::Tpm_alg::Type::SM3_256:  bmp |= Type::SM3_256;  break;
                case Tcg::Tpm_alg::Type::SHA3_256: bmp |= Type::SHA3_256; break;
                case Tcg::Tpm_alg::Type::SHA3_384: bmp |= Type::SHA3_384; break;
                case Tcg::Tpm_alg::Type::SHA3_512: bmp |= Type::SHA3_512; break;
            }
        }

        auto count() const { return static_cast<unsigned>(__builtin_popcount (bmp)); }

        bool supported (Type t) const { return bmp & t; }
};
