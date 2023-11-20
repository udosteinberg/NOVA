/*
 * Secure Hash Algorithm (SHA)
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

#include "bits.hpp"
#include "endian.hpp"

/*
 * NIST FIPS PUB 180-4
 */
class Fips180
{
    private:
        size_t len {};

        // Section 4.1.1
        template<typename T> static constexpr T par (T x, T y, T z) { return x ^ y ^ z; }
        template<typename T> static constexpr T chx (T x, T y, T z) { return z ^ (x & (y ^ z)); }
        template<typename T> static constexpr T maj (T x, T y, T z) { return z ^ ((x ^ z) & (y ^ z)); }

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

        // Section 4.2: The first 64 bits of the fractional parts of the cube roots of the first 80 prime numbers
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

    protected:
        // Largest block size (SHA2-384, SHA2-512)
        static constexpr unsigned blk_max { 16 * sizeof (uint64_t) };

        // Section 5.1, 5.2
        __attribute__((nonnull, noinline)) void absorb (void *h, uint8_t const *m, size_t s, bool more, void (*block)(void *, uint8_t const *), unsigned w)
        {
            unsigned const res_size { 14 * w }, blk_size { 16 * w };

            // Update message length
            len += s;

            // Process full blocks
            for (; s >= blk_size; s -= blk_size, m += blk_size)
                block (h, m);

            // If more blocks follow, then s must have been a multiple of blk_size and is now 0
            if (more)
                return;

            // Process partial block with s residual bytes
            uint8_t b[blk_max];
            __builtin_memcpy (b, m, s);
            b[s++] = BIT (7);

            // Extra block if the length info does not fit
            if (s > res_size) [[unlikely]] {

                // Zero-pad the rest of the block
                while (s < blk_size)
                    b[s++] = 0;

                block (h, b);

                s = 0;
            }

            // Zero-pad up to the length info
            __builtin_memset (b + s, 0, res_size - s);

            // Append the length info in bits to the last block
            if (w == sizeof (uint64_t)) {
                using L = uint128_t;
                *std::start_lifetime_as<Unaligned_be<L>> (b + res_size) = L { len } * __CHAR_BIT__;
            } else {
                using L = uint64_t;
                *std::start_lifetime_as<Unaligned_be<L>> (b + res_size) = L { len } * __CHAR_BIT__;
            }

            block (h, b);
        }

        // Section 6.1
        template<typename T> __attribute__((nonnull)) static void sha1 (void *s, uint8_t const *m)
        {
            auto const h { static_cast<T *>(s) };
            auto const p { std::start_lifetime_as_array<Unaligned_be<T> const> (m, 16) };

            // Initialize working variables with previous hash value
            T w[16], v[5] { h[0], h[1], h[2], h[3], h[4] };

            // Compression: W[i] depends on nothing older than W[i - 16], so a 16-word circular buffer suffices
            for (unsigned i { 0 }; i < 80; i++) {

                T t { (w[i % 16] = i < 16 ? T { p[i] } : rol (w[(i - 3) % 16] ^ w[(i - 8) % 16] ^ w[(i - 14) % 16] ^ w[i % 16], 1)) + rol (v[0], 5) + v[4] };

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

        // Section 6.2, 6.3, 6.4, 6.5
        template<typename T> __attribute__((nonnull)) static void sha2 (void *s, uint8_t const *m)
        {
            auto const h { static_cast<T *>(s) };
            auto const p { std::start_lifetime_as_array<Unaligned_be<T> const> (m, 16) };

            // Initialize working variables with previous hash value
            T w[16], v[8] { h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7] };

            // Compression: W[i] depends on nothing older than W[i - 16], so a 16-word circular buffer suffices
            for (unsigned i { 0 }; i < 4 * sizeof (T) + 48; i++) {

                T const t1 { (w[i % 16] = i < 16 ? T { p[i] } : sig1 (w[(i - 2) % 16]) + w[(i - 7) % 16] + sig0 (w[(i - 15) % 16]) + w[i % 16]) + v[7] + sum1 (v[4]) + chx (v[4], v[5], v[6]) + static_cast<T>(cbr[i] >> (64 - __CHAR_BIT__ * sizeof (T))) };
                T const t2 { sum0 (v[0]) + maj (v[0], v[1], v[2]) };

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

        template<typename T> __attribute__((nonnull)) static void squeeze (T const *h, uint8_t *p, unsigned s)
        {
            auto const n { s / sizeof (T) };
            auto const d { std::start_lifetime_as_array<Unaligned_be<T>> (p, n) };

            for (unsigned i { 0 }; i < n; i++)
                d[i] = h[i];
        }
};

template<uint16_t H, unsigned D, typename T, T... I> requires ((__is_same (T, uint32_t) || __is_same (T, uint64_t)) && sizeof...(I) == (D == 160 ? 5 : 8) && (D != 160 || type_bits<T>() == 32) && D % type_bits<T>() == 0 && D <= type_bits<T>() * sizeof...(I)) class Sha1_2 : private Fips180
{
    private:
        T h[sizeof...(I)] { I... };

    public:
        static constexpr auto tpm_hash { H };                           // TPM Hash Algorithm ID
        static constexpr auto dig_size { D / __CHAR_BIT__ };            // Digest Size
        static constexpr auto blk_size { 16 * sizeof (T) };             // Block Size

        static_assert (dig_size <= blk_size && blk_size <= blk_max);

        __attribute__((nonnull)) auto &update (uint8_t const *m, size_t s, bool more = false) { absorb (h, m, s, more, D == 160 ? sha1<T> : sha2<T>, sizeof (T)); return *this; }

        __attribute__((nonnull)) void digest (uint8_t *p) const { squeeze (h, p, dig_size); }
};

// Section 5.3
using Hash_sha1_160 = Sha1_2<0x4, 160, uint32_t, 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0>;
using Hash_sha2_256 = Sha1_2<0xb, 256, uint32_t, 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19>;
using Hash_sha2_384 = Sha1_2<0xc, 384, uint64_t, 0xcbbb9d5dc1059ed8, 0x629a292a367cd507, 0x9159015a3070dd17, 0x152fecd8f70e5939, 0x67332667ffc00b31, 0x8eb44a8768581511, 0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4>;
using Hash_sha2_512 = Sha1_2<0xd, 512, uint64_t, 0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1, 0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179>;

// Sanity Checks
static_assert (Hash_sha1_160::blk_size ==  64);
static_assert (Hash_sha2_256::blk_size ==  64);
static_assert (Hash_sha2_384::blk_size == 128);
static_assert (Hash_sha2_512::blk_size == 128);

/*
 * NIST FIPS PUB 202
 */
class Fips202
{
    private:
        // Section 3.2.5, Algorithm 5: XOR value for IOTA precomputed from LFSR bits
        static constexpr uint64_t rnd[24]
        {
            0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000, 0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
            0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a, 0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003,
            0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a, 0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008,
        };

        // Section 3.3, Algorithm 7: KECCAK-p[1600, 24]
        static void permute (uint64_t (&h)[25])
        {
            for (auto const r : rnd) {

                // Section 3.2.1, Algorithm 1: THETA
                uint64_t const c[5]
                {
                    h[0] ^ h[5] ^ h[10] ^ h[15] ^ h[20],
                    h[1] ^ h[6] ^ h[11] ^ h[16] ^ h[21],
                    h[2] ^ h[7] ^ h[12] ^ h[17] ^ h[22],
                    h[3] ^ h[8] ^ h[13] ^ h[18] ^ h[23],
                    h[4] ^ h[9] ^ h[14] ^ h[19] ^ h[24],
                };

                uint64_t const d[5]
                {
                    c[4] ^ rol (c[1], 1),
                    c[0] ^ rol (c[2], 1),
                    c[1] ^ rol (c[3], 1),
                    c[2] ^ rol (c[4], 1),
                    c[3] ^ rol (c[0], 1),
                };

                for (unsigned i { 0 }; i < sizeof (h) / sizeof (*h); i += 5) {
                    h[i + 0] ^= d[0];
                    h[i + 1] ^= d[1];
                    h[i + 2] ^= d[2];
                    h[i + 3] ^= d[3];
                    h[i + 4] ^= d[4];
                }

                // Section 3.2.2, 3.2.3, Algorithm 2 + 3: RHO fused with PI
                uint64_t l { h[1] }, t;
                t = h[10]; h[10] = rol (l,  1); l = t;
                t = h[ 7]; h[ 7] = rol (l,  3); l = t;
                t = h[11]; h[11] = rol (l,  6); l = t;
                t = h[17]; h[17] = rol (l, 10); l = t;
                t = h[18]; h[18] = rol (l, 15); l = t;
                t = h[ 3]; h[ 3] = rol (l, 21); l = t;
                t = h[ 5]; h[ 5] = rol (l, 28); l = t;
                t = h[16]; h[16] = rol (l, 36); l = t;
                t = h[ 8]; h[ 8] = rol (l, 45); l = t;
                t = h[21]; h[21] = rol (l, 55); l = t;
                t = h[24]; h[24] = rol (l,  2); l = t;
                t = h[ 4]; h[ 4] = rol (l, 14); l = t;
                t = h[15]; h[15] = rol (l, 27); l = t;
                t = h[23]; h[23] = rol (l, 41); l = t;
                t = h[19]; h[19] = rol (l, 56); l = t;
                t = h[13]; h[13] = rol (l,  8); l = t;
                t = h[12]; h[12] = rol (l, 25); l = t;
                t = h[ 2]; h[ 2] = rol (l, 43); l = t;
                t = h[20]; h[20] = rol (l, 62); l = t;
                t = h[14]; h[14] = rol (l, 18); l = t;
                t = h[22]; h[22] = rol (l, 39); l = t;
                t = h[ 9]; h[ 9] = rol (l, 61); l = t;
                t = h[ 6]; h[ 6] = rol (l, 20); l = t;
                t = h[ 1]; h[ 1] = rol (l, 44); l = t;

                // Section 3.2.4, Algorithm 4: CHI
                for (unsigned i { 0 }; i < sizeof (h) / sizeof (*h); i += 5) {
                    uint64_t const p[5] { h[i + 0], h[i + 1], h[i + 2], h[i + 3], h[i + 4] };
                    h[i + 0] = p[0] ^ (~p[1] & p[2]);
                    h[i + 1] = p[1] ^ (~p[2] & p[3]);
                    h[i + 2] = p[2] ^ (~p[3] & p[4]);
                    h[i + 3] = p[3] ^ (~p[4] & p[0]);
                    h[i + 4] = p[4] ^ (~p[0] & p[1]);
                }

                // Section 3.2.5, Algorithm 6: IOTA
                h[0] ^= r;
            }
        }

        // Section 4, Algorithm 8: SPONGE
        __attribute__((nonnull)) static void block (uint64_t (&h)[25], uint8_t const *m, size_t blk_size)
        {
            auto const n { blk_size / sizeof (uint64_t) };
            auto const p { std::start_lifetime_as_array<Unaligned_le<uint64_t> const> (m, n) };

            for (unsigned i { 0 }; i < n; i++)
                h[i] ^= p[i];

            permute (h);
        }

    protected:
        // Largest block size (SHA3-256)
        static constexpr unsigned blk_max { 136 };

        // Section 4, Algorithm 8: SPONGE
        __attribute__((nonnull, noinline)) static void absorb (uint64_t (&h)[25], uint8_t const *m, size_t s, bool more, size_t blk_size)
        {
            // Process full blocks
            for (; s >= blk_size; s -= blk_size, m += blk_size)
                block (h, m, blk_size);

            // If more blocks follow, then s must have been a multiple of blk_size and is now 0
            if (more)
                return;

            // Process partial block with s residual bytes
            uint8_t b[blk_max];
            __builtin_memcpy (b, m, s);
            __builtin_memset (b + s, 0, blk_size - s);

            // Section 5.1, 6.1
            b[s] ^= 0x06;
            b[blk_size - 1] ^= 0x80;

            block (h, b, blk_size);
        }

        __attribute__((nonnull)) static void squeeze (uint64_t const (&h)[25], uint8_t *p, unsigned s)
        {
            auto const n { s / sizeof (uint64_t) };
            auto const d { std::start_lifetime_as_array<Unaligned_le<uint64_t>> (p, n) };

            for (unsigned i { 0 }; i < n; i++)
                d[i] = h[i];
        }
};

template<uint16_t H, unsigned D> requires (D == 256 || D == 384 || D == 512) class Sha3 : private Fips202
{
    private:
        uint64_t h[25] {};

    public:
        static constexpr auto tpm_hash { H };                           // TPM Hash Algorithm ID
        static constexpr auto dig_size { D / __CHAR_BIT__ };            // Digest Size
        static constexpr auto blk_size { sizeof (h) - 2 * dig_size };   // Block Size (Rate)

        static_assert (dig_size <= blk_size && blk_size <= blk_max);

        __attribute__((nonnull)) auto &update (uint8_t const *m, size_t s, bool more = false) { absorb (h, m, s, more, blk_size); return *this; }

        __attribute__((nonnull)) void digest (uint8_t *p) const { squeeze (h, p, dig_size); }
};

// Section 6.1
using Hash_sha3_256 = Sha3<0x27, 256>;
using Hash_sha3_384 = Sha3<0x28, 384>;
using Hash_sha3_512 = Sha3<0x29, 512>;

// Sanity Checks
static_assert (Hash_sha3_256::blk_size == 136);
static_assert (Hash_sha3_384::blk_size == 104);
static_assert (Hash_sha3_512::blk_size ==  72);
