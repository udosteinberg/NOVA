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

#include "space_hst.hpp"
#include "tpm_log.hpp"

void Tpm_log::init (uint64_t p, uint32_t s, uint32_t o)
{
    // Check for valid log parameters
    if (p != phys() || s != size || o > s) [[unlikely]]
        return;

    // Check for valid log header
    auto const h { std::start_lifetime_as<Header const> (virt()) };
    if (!h->valid()) [[unlikely]]
        return;

    auto const cnt { h->cnt };
    auto const esz { h->esz };

    // Check for reasonable number of hash algorithms
    if (!cnt || cnt > type_bits<Hash_bmp>()) [[unlikely]]
        return;

    // Check for valid event size and offset beyond log header
    auto a { std::start_lifetime_as_array<Algorithm const> (h + 1, cnt) };
    if (esz != 29 + cnt * sizeof (Algorithm) + *std::start_lifetime_as<uint8_t const> (a + cnt) || o < 32 + esz) [[unlikely]]
        return;

    // Check for valid digest sizes
    for (auto i { cnt }; i--; )
        if (a[i].dsz != Hash_bmp::dig_size (a[i].alg)) [[unlikely]]
            return;

    // Reinitialize log state every time this function runs
    offs = o;
    tdsz = 0;
    hash = {};

    // Update hash registry with supported hash algorithms
    for (auto i { cnt }; i--; a++)
        if (hash.add (a->alg)) [[likely]]
            tdsz += sizeof (a->alg) + a->dsz;

    // Grant user read access to the log
    Space_hst::access_ctrl (phys(), size, Paging::Permissions (Paging::U | Paging::R));

    trace (TRACE_DRTM, "DRTM: TPM-LOG v%u: %4u", h->maj, offs);
}

bool Tpm_log::extend (uint32_t eid, uint32_t pcr, Hash_sha1_160 const &sha1_160, Hash_sha2_256 const &sha2_256, Hash_sha2_384 const &sha2_384, Hash_sha2_512 const &sha2_512, Hash_sha3_256 const &sha3_256, Hash_sha3_384 const &sha3_384, Hash_sha3_512 const &sha3_512)
{
    // Compute required length including all tagged digests
    auto const len { static_cast<uint32_t>(6 * sizeof (uint32_t) + tdsz) };

    // Check for initialized log with sufficient space
    if (!offs || len > size - offs) [[unlikely]]
        return false;

    // Compute log entry pointer
    auto p { std::start_lifetime_as_array<uint8_t> (virt() + offs, len) };

    new (p + 0) Unaligned_le<uint32_t> { pcr };                                         // pcrIndex
    new (p + 4) Unaligned_le<uint32_t> { std::to_underlying (Event::EV_EVENT_TAG) };    // eventType
    new (p + 8) Unaligned_le<uint32_t> { hash.count() };                                // digestCount

    p += 3 * sizeof (uint32_t);

    // Add digests for supported hash algorithms
    add_digest (p, Hash_bmp::Type::SHA1_160, sha1_160);
    add_digest (p, Hash_bmp::Type::SHA2_256, sha2_256);
    add_digest (p, Hash_bmp::Type::SHA2_384, sha2_384);
    add_digest (p, Hash_bmp::Type::SHA2_512, sha2_512);
    add_digest (p, Hash_bmp::Type::SHA3_256, sha3_256);
    add_digest (p, Hash_bmp::Type::SHA3_384, sha3_384);
    add_digest (p, Hash_bmp::Type::SHA3_512, sha3_512);

    // Add digests for unsupported hash algorithms
    add_digest (p, Hash_bmp::Type::SM3_256, Tcg::Tpm_alg_id::Hash::SM3_256, Hash_bmp::dig_size (Tcg::Tpm_alg_id::Hash::SM3_256));

    new (p + 0) Unaligned_le<uint32_t> { 8 };                                           // eventSize
    new (p + 4) Unaligned_le<uint32_t> { eid };                                         // taggedEventID
    new (p + 8) Unaligned_le<uint32_t> { 0 };                                           // taggedEventDataSize

    offs += len;

    return true;
}
