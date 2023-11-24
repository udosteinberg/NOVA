/*
 * Integrity Measurement
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

#include "integrity.hpp"
#include "ptab_hst.hpp"

bool Integrity::measure (uint64_t phys, size_t size)
{
    // Chunk size must be a multiple of every block size
    constexpr auto chunk_size { Pte_hst::page_size (Pte_hst::bpl) - Pte_hst::page_size (Pte_hst::bpl) % lcm (Hash_sha1_160::blk_size, Hash_sha2_256::blk_size, Hash_sha2_384::blk_size, Hash_sha2_512::blk_size, Hash_sha3_256::blk_size, Hash_sha3_384::blk_size, Hash_sha3_512::blk_size) };

    Hash_sha1_160 sha1_160;
    Hash_sha2_256 sha2_256;
    Hash_sha2_384 sha2_384;
    Hash_sha2_512 sha2_512;
    Hash_sha3_256 sha3_256;
    Hash_sha3_384 sha3_384;
    Hash_sha3_512 sha3_512;

    // Even for size 0 the loop must execute once to yield a valid digest
    for (uint64_t p { phys }, s { size };; p += chunk_size) {

        auto const ptr { static_cast<uint8_t const *>(Ptab_hst::map (MMAP_GLB_MAP0, p)) };
        auto const len { min (s, chunk_size) };
        s -= len;

        sha1_160.update (ptr, len, s);
        sha2_256.update (ptr, len, s);
        sha2_384.update (ptr, len, s);
        sha2_512.update (ptr, len, s);
        sha3_256.update (ptr, len, s);
        sha3_384.update (ptr, len, s);
        sha3_512.update (ptr, len, s);

        if (!s)
            break;
    }

    return extend (sha1_160, sha2_256, sha2_384, sha2_512, sha3_256, sha3_384, sha3_512);
}
