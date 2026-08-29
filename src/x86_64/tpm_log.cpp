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

#include "ptab_hpt.hpp"
#include "signature.hpp"
#include "tpm_log.hpp"

void Tpm_log::init (uint64_t p, uint32_t s, uint32_t o)
{
    // Check for valid parameters. s/o must also fit within the single page
    // mapped below at MMAP_GLB_MAP0, since extend() later maps the same
    // page and writes at that offset without re-checking against it.
    if (!p || !s || !o || o > s || s > Hpt::page_size (Hpt::bpl)) [[unlikely]]
        return;

    // Map page-aligned header
    auto const h { std::start_lifetime_as<Header const> (Hptp::map (MMAP_GLB_MAP0, p)) };

    // Check for valid header
    if (h->pcr || h->evt != Event::EV_NO_ACTION || h->cnt > type_bits<Hash_bmp>() || h->esz < 29 + h->cnt * sizeof (Algorithm)) [[unlikely]]
        return;

    // Check for valid signature
    constexpr auto id { "Spec ID Event03" };
    if (h->sig[0] != Signature::u32 (id) || h->sig[1] != Signature::u32 (id + 4) || h->sig[2] != Signature::u32 (id + 8) || h->sig[3] != Signature::u32 (id + 12)) [[unlikely]]
        return;

    phys = p;
    size = s;
    offs = o;

    // Reset hash registry every time this function runs
    tdsz = 0;
    hash = {};

    // Update hash registry with supported hash algorithms
    auto a { std::start_lifetime_as_array<Algorithm const> (h + 1, h->cnt) };
    for (unsigned i { h->cnt }; i--; tdsz += sizeof (a->alg) + a->dsz, a++)
        hash.add (a->alg);

    trace (TRACE_DRTM, "DRTM: TPM-LOG v%u: %4u", h->maj, offs);
}

bool Tpm_log::extend (unsigned pcr, Hash_sha1_160 const &sha1_160, Hash_sha2_256 const &sha2_256, Hash_sha2_384 const &sha2_384, Hash_sha2_512 const &sha2_512)
{
    // Compute entry length including all tagged digests
    auto const len { static_cast<uint32_t>(6 * sizeof (uint32_t) + tdsz) };

    // Check for sufficient space
    if (len > size - offs) [[unlikely]]
        return false;

    // Map unaligned log storage
    auto p { static_cast<uint8_t *>(Hptp::map (MMAP_GLB_MAP0, phys, Paging::W)) + offs };

    new (p + 0) Unaligned_le<uint32_t> { pcr };                                         // pcrIndex
    new (p + 4) Unaligned_le<uint32_t> { std::to_underlying (Event::EV_EVENT_TAG) };    // eventType
    new (p + 8) Unaligned_le<uint32_t> { hash.count() };                                // digestCount

    p += 3 * sizeof (uint32_t);

    add_digest (Hash_bmp::Type::SHA1_160, sha1_160, p);
    add_digest (Hash_bmp::Type::SHA2_256, sha2_256, p);
    add_digest (Hash_bmp::Type::SHA2_384, sha2_384, p);
    add_digest (Hash_bmp::Type::SHA2_512, sha2_512, p);

    new (p + 0) Unaligned_le<uint32_t> { 8 };                                           // eventSize
    new (p + 4) Unaligned_le<uint32_t> { 0 };                                           // taggedEventID
    new (p + 8) Unaligned_le<uint32_t> { 0 };                                           // taggedEventDataSize

    offs += len;

    return true;
}
