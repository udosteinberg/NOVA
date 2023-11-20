/*
 * Secure Hash Algorithm (SHA)
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

#include "byteorder.hpp"
#include "macros.hpp"
#include "std.hpp"
#include "string.hpp"

class Sha
{
    private:
        size_t len { 0 };

        // Section 3.2
        template <typename T> static constexpr T shr (T x, unsigned n) { return x >> n; }
        template <typename T> static constexpr T rol (T x, unsigned n) { return x << n | x >> (-n & (8 * sizeof (T) - 1)); }
        template <typename T> static constexpr T ror (T x, unsigned n) { return x >> n | x << (-n & (8 * sizeof (T) - 1)); }

        // Section 4.1
        template <typename T> static constexpr T chx (T x, T y, T z) { return (x & y) ^ (~x & z); }
        template <typename T> static constexpr T maj (T x, T y, T z) { return (x & y) ^ (x & z) ^ (y & z); }
        template <typename T> static constexpr T par (T x, T y, T z) { return x ^ y ^ z; }

        // Section 4.1.2
        static constexpr uint32_t sum0 (uint32_t x) { return ror (x,  2) ^ ror (x, 13) ^ ror (x, 22); }
        static constexpr uint32_t sum1 (uint32_t x) { return ror (x,  6) ^ ror (x, 11) ^ ror (x, 25); }
        static constexpr uint32_t sig0 (uint32_t x) { return ror (x,  7) ^ ror (x, 18) ^ shr (x,  3); }
        static constexpr uint32_t sig1 (uint32_t x) { return ror (x, 17) ^ ror (x, 19) ^ shr (x, 10); }

        // Section 4.1.3
        static constexpr uint64_t sum0 (uint64_t x) { return ror (x, 28) ^ ror (x, 34) ^ ror (x, 39); }
        static constexpr uint64_t sum1 (uint64_t x) { return ror (x, 14) ^ ror (x, 18) ^ ror (x, 41); }
        static constexpr uint64_t sig0 (uint64_t x) { return ror (x,  1) ^ ror (x,  8) ^ shr (x,  7); }
        static constexpr uint64_t sig1 (uint64_t x) { return ror (x, 19) ^ ror (x, 61) ^ shr (x,  6); }

    protected:
        // The first 64 bits of the fractional parts of the cube roots of the first 80 prime numbers
        static constexpr uint64_t cbr[80]
        {
            0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
            0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
            0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65, 0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
            0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
            0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
            0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
            0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
            0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec, 0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
            0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
            0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817,
        };

        template <typename T>
        static void sha1 (T *h, uint8_t const *m)
        {
            T w[80];

            // Prepare message schedule
            for (unsigned i { 0 }; i < 16; i++, m += sizeof (T))
                w[i] = Byteorder_be<T> { m };

            for (unsigned i { 16 }; i < sizeof (w) / sizeof (*w); i++)
                w[i] = rol (w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

            // Initialize working variables with previous hash value
            T v[5] { h[0], h[1], h[2], h[3], h[4] }, t;

            // Compression
            for (unsigned i { 0 }; i < sizeof (w) / sizeof (*w); i++) {

                t = w[i] + rol (v[0], 5) + v[4];

                if (i < 20)
                    t += chx (v[1], v[2], v[3]) + 0x5a827999;
                else if (i < 40)
                    t += par (v[1], v[2], v[3]) + 0x6ed9eba1;
                else if (i < 60)
                    t += maj (v[1], v[2], v[3]) + 0x8f1bbcdc;
                else
                    t += par (v[1], v[2], v[3]) + 0xca62c1d6;

                v[4] = v[3];
                v[3] = v[2];
                v[2] = rol (v[1], 30);
                v[1] = v[0];
                v[0] = t;
            }

            // Update intermediate hash value
            for (unsigned i { 0 }; i < sizeof (v) / sizeof (*v); i++)
                h[i] += v[i];
        }

        template <typename T>
        static void sha2 (T *h, uint8_t const *m)
        {
            T w[4 * sizeof (T) + 48];

            // Prepare message schedule
            for (unsigned i { 0 }; i < 16; i++, m += sizeof (T))
                w[i] = Byteorder_be<T> { m };

            for (unsigned i { 16 }; i < sizeof (w) / sizeof (*w); i++)
                w[i] = sig1 (w[i - 2]) + w[i - 7] + sig0 (w[i - 15]) + w[i - 16];

            // Initialize working variables with previous hash value
            T v[8] { h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7] }, t1, t2;

            // Compression
            for (unsigned i { 0 }; i < sizeof (w) / sizeof (*w); i++) {

                t1 = w[i] + v[7] + sum1 (v[4]) + chx (v[4], v[5], v[6]) + static_cast<T>(cbr[i] >> (64 - 8 * sizeof (T)));
                t2 = sum0 (v[0]) + maj (v[0], v[1], v[2]);

                v[7] = v[6];
                v[6] = v[5];
                v[5] = v[4];
                v[4] = v[3] + t1;
                v[3] = v[2];
                v[2] = v[1];
                v[1] = v[0];
                v[0] = t1 + t2;
            }

            // Update intermediate hash value
            for (unsigned i { 0 }; i < sizeof (v) / sizeof (*v); i++)
                h[i] += v[i];
        }

        template <typename T>
        void preprocess (T *h, uint8_t const *m, size_t s, bool more, void (*hash)(T *, uint8_t const *))
        {
            constexpr auto res_size { 14 * sizeof (T) }, blk_size { 16 * sizeof (T) };

            // Update message length
            len += s;

            // Process full blocks
            for (; s >= blk_size; s -= blk_size, m += blk_size)
                hash (h, m);

            // If more blocks follow, then s must have been a multiple of blk_size and is now 0
            if (more)
                return;

            // Process partial block with s residual bytes
            uint8_t b[blk_size];
            memcpy (b, m, s);
            b[s++] = BIT (7);

            // Extra block if the length info does not fit
            if (s > res_size) {

                // Zero-pad the rest of the block
                while (s < blk_size)
                    b[s++] = 0;

                hash (h, b);

                s = 0;
            }

            // Zero-pad up to the length info
            memset (b + s, 0, res_size - s);

            using L = std::conditional<sizeof (T) == sizeof (uint64_t), uint128_t, uint64_t>::type;

            // Append the length info in bits to the last block
            Byteorder_be { 8 * static_cast<L>(len) }.serialize (b + res_size);

            hash (h, b);
        }
};

template <unsigned D, unsigned H, typename T, void (*F)(T *, uint8_t const *), T... I>
class Hash : private Sha
{
    private:
        T h[H] { I... };

    public:
        static constexpr auto digest { D };

        auto serialize (uint8_t *p) const
        {
            for (unsigned i { 0 }; i < digest / sizeof (T); i++, p += sizeof (T))
                Byteorder_be { h[i] }.serialize (p);

            return p;
        }

        void update (uint8_t const *m, size_t s, bool more = false) { preprocess (h, m, s, more, F); }
};

class Hash_sha1_160 final : public Hash<20, 5, uint32_t, Sha::sha1, 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0> {};
class Hash_sha2_224 final : public Hash<28, 8, uint32_t, Sha::sha2, 0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4> {};
class Hash_sha2_256 final : public Hash<32, 8, uint32_t, Sha::sha2, 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19> {};
class Hash_sha2_384 final : public Hash<48, 8, uint64_t, Sha::sha2, 0xcbbb9d5dc1059ed8, 0x629a292a367cd507, 0x9159015a3070dd17, 0x152fecd8f70e5939, 0x67332667ffc00b31, 0x8eb44a8768581511, 0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4> {};
class Hash_sha2_512 final : public Hash<64, 8, uint64_t, Sha::sha2, 0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1, 0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179> {};
