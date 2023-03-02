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

#include "ptab_hpt.hpp"
#include "signature.hpp"
#include "tpm_log.hpp"

void Tpm_log::init (uint64_t p, uint32_t s, uint32_t o)
{
    // Check for valid parameters
    if (EXPECT_FALSE (!p || !s || !o))
        return;

    // Map page-aligned header
    auto const h { static_cast<Header const *>(Hptp::map (MMAP_GLB_MAP0, p)) };

    // Check for valid header
    if (EXPECT_FALSE (h->pcr || h->evt != Event::EV_NO_ACTION || h->esz != 29 + h->cnt * sizeof (Algorithm)))
        return;

    // Check for valid signature
    constexpr auto id { "Spec ID Event03" };
    if (EXPECT_FALSE (h->sig[0] != Signature::u32 (id) || h->sig[1] != Signature::u32 (id + 4) || h->sig[2] != Signature::u32 (id + 8) || h->sig[3] != Signature::u32 (id + 12)))
        return;

    phys = p;
    size = s;
    offs = o;

    // Enumerate supported hash algorithms
    auto a { reinterpret_cast<Algorithm const *>(h + 1) };
    for (unsigned i { h->cnt }; i--; tdsz += sizeof (a->alg) + a->dsz, a++)
        hash.add (a->alg);

    trace (TRACE_DRTM, "DRTM: TPM-LOG v%u: %4u", h->maj, offs);
}

bool Tpm_log::extend (unsigned pcr, Hash_sha1_160 const &sha1_160, Hash_sha2_256 const &sha2_256, Hash_sha2_384 const &sha2_384, Hash_sha2_512 const &sha2_512)
{
    // Compute entry length including all tagged digests
    auto const len { static_cast<uint32_t>(6 * sizeof (le_uint32_t) + tdsz) };

    // Check for sufficient space
    if (EXPECT_FALSE (len > size - offs))
        return false;

    // Map unaligned log storage
    auto p { static_cast<uint8_t *>(Hptp::map (MMAP_GLB_MAP0, phys, Paging::W)) + offs };

    auto const evt { std::to_underlying (Event::EV_EVENT_TAG) };
    auto const cnt { hash.count() };

    le_uint32_t { pcr }.serialize (p + 0);          // pcrIndex;
    le_uint32_t { evt }.serialize (p + 4);          // eventType;
    le_uint32_t { cnt }.serialize (p + 8);          // digestCount;
    p += 3 * sizeof (le_uint32_t);

    add_digest (Hash_bmp::Type::SHA1_160, Tcg::Tpm_alg::Type::SHA1_160, sha1_160, p);
    add_digest (Hash_bmp::Type::SHA2_256, Tcg::Tpm_alg::Type::SHA2_256, sha2_256, p);
    add_digest (Hash_bmp::Type::SHA2_384, Tcg::Tpm_alg::Type::SHA2_384, sha2_384, p);
    add_digest (Hash_bmp::Type::SHA2_512, Tcg::Tpm_alg::Type::SHA2_512, sha2_512, p);

    le_uint32_t { 8U }.serialize (p + 0);           // eventSize
    le_uint32_t { 0U }.serialize (p + 4);           // taggedEventID
    le_uint32_t { 0U }.serialize (p + 8);           // taggedEventDataSize
    p += 3 * sizeof (le_uint32_t);

    offs += len;

    return true;
}
