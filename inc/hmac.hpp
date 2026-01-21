/*
 * Keyed-Hash Message Authentication Code (HMAC)
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

#include "hash.hpp"

/*
 * NIST FIPS PUB 198-1
 */
template<typename H> requires (H::dig_size <= H::blk_size) class Hmac
{
    private:
        // Precomputed intermediate results of the compression function for blocks (K0 ^ ipad) and (K0 ^ opad)
        struct P { H ipad, opad; } const key;

        static P precompute (uint8_t const *k, size_t s)
        {
            uint8_t ipad[H::blk_size], opad[H::blk_size];

            // Step 1 + 2 + 3: Create K0 in opad
            if (s > H::blk_size) [[unlikely]] {
                H {}.update (k, s).digest (opad);
                s = H::dig_size;
            } else if (s) [[likely]]
                __builtin_memcpy (opad, k, s);

            // Zero-pad K0 to block size
            __builtin_memset (opad + s, 0, H::blk_size - s);

            // Step 4 + 7
            for (unsigned i { 0 }; i < H::blk_size; i++) {
                ipad[i] = opad[i] ^ 0x36;
                opad[i] = opad[i] ^ 0x5c;
            }

            // Step 6 + 9 (intermediate)
            return { std::move (H {}.update (ipad, H::blk_size, true)), std::move (H {}.update (opad, H::blk_size, true)) };
        }

        // Target Constructor
        explicit constexpr Hmac (P k) : key { std::move (k) } {}

    public:
        void compute (uint8_t (&hmac)[H::dig_size], uint8_t const *m, size_t s) const
        {
            // Step 5 + 6
            H { key.ipad }.update (m, s).digest (hmac);

            // Step 8 + 9
            H { key.opad }.update (hmac, H::dig_size).digest (hmac);
        }

        // Delegating Constructor
        explicit Hmac (uint8_t const *k, size_t s) : Hmac { precompute (k, s) } {}
};

using Hmac_sha1_160 = Hmac<Hash_sha1_160>;
using Hmac_sha2_224 = Hmac<Hash_sha2_224>;
using Hmac_sha2_256 = Hmac<Hash_sha2_256>;
using Hmac_sha2_384 = Hmac<Hash_sha2_384>;
using Hmac_sha2_512 = Hmac<Hash_sha2_512>;
