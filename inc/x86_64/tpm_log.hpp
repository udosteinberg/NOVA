/*
 * Trusted Platform Module (TPM) Crypto Agile Event Log
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

#include "signature.hpp"
#include "tpm_alg.hpp"

class Tpm_log final
{
    friend class Hip_arch;

    private:
        static inline constinit uint32_t offs { 0 };
        static inline constinit size_t   tdsz { 0 };
        static inline constinit Hash_bmp hash;

        enum class Event : uint32_t
        {
            EV_NO_ACTION        = 0x3,
            EV_EVENT_TAG        = 0x6,
        };

        struct Header
        {
            uint32_t            pcr;                            //  0   PCR Index
            Event               evt;                            //  4   Event Type
            uint8_t             dig[Hash_sha1_160::dig_size];   //  8   Digest
            uint32_t            esz;                            // 28   Event Size
            uint32_t            sig[4];                         // 32   Signature
            uint32_t            cls;                            // 48   Platform Class
            uint8_t             min, maj, err, usz;             // 52   Spec Minor, Major, Errata, UINTN size
            uint32_t            cnt;                            // 56   Number of Hash Algorithms

            bool valid() const
            {
                constexpr auto id { "Spec ID Event03" };

                return pcr == 0 && evt == Event::EV_NO_ACTION && sig[0] == Signature::u32 (id) && sig[1] == Signature::u32 (id + 4) && sig[2] == Signature::u32 (id + 8) && sig[3] == Signature::u32 (id + 12);
            }
        };

        static_assert (__is_standard_layout (Header) && alignof (Header) == 4 && sizeof (Header) == 60);

        struct Algorithm
        {
            Tcg::Tpm_alg_id::Hash   alg;                        //  0
            uint16_t                dsz;                        //  2
        };

        static_assert (__is_standard_layout (Algorithm) && sizeof (Algorithm) == 4);

        [[nodiscard]] static bool add_alg_id (uint8_t *&p, Hash_bmp::Type h, Tcg::Tpm_alg_id::Hash a)
        {
            // Ignore unsupported hash algorithm
            if (!hash.supported (h)) [[unlikely]]
                return false;

            // Add algorithm identifier
            new (p) Unaligned_le<uint16_t> { std::to_underlying (a) };
            p += sizeof (a);

            return true;
        }

        static void add_digest (uint8_t *&p, Hash_bmp::Type h, auto const &d)
        {
            if (add_alg_id (p, h, Tcg::Tpm_alg_id::Hash { d.tpm_hash })) [[likely]] {
                d.digest (p);
                p += d.dig_size;
            }
        }

        static void add_digest (uint8_t *&p, Hash_bmp::Type h, Tcg::Tpm_alg_id::Hash a, uint32_t l)
        {
            if (add_alg_id (p, h, a)) [[likely]] {
                Tcg::digest_cap (p, l);
                p += l;
            }
        }

    public:
        static constexpr uint32_t size { PAGE_SIZE (0) };

        static auto phys() { return Kmem::sym_to_phys (&MLE_TL); }
        static auto virt() { return Kmem::sym_to_virt (&MLE_TL); }

        static void init (uint64_t, uint32_t, uint32_t);

        [[nodiscard]] static bool extend (uint32_t, uint32_t, Hash_sha1_160 const &, Hash_sha2_256 const &, Hash_sha2_384 const &, Hash_sha2_512 const &, Hash_sha3_256 const &, Hash_sha3_384 const &, Hash_sha3_512 const &);
};
