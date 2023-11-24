/*
 * Integrity Measurement
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

#include "tpm.hpp"
#include "tpm_log.hpp"
#include "txt.hpp"

class Integrity final
{
    private:
        static bool extend (unsigned pcr, Hash_sha1_160 const &sha1_160, Hash_sha2_256 const &sha2_256, Hash_sha2_384 const &sha2_384, Hash_sha2_512 const &sha2_512)
        {
            return Txt::launched && Tpm::extend (pcr, sha1_160, sha2_256, sha2_384, sha2_512) && Tpm_log::extend (pcr, sha1_160, sha2_256, sha2_384, sha2_512);
        }

    public:
        static inline uint64_t root_phys { 0 }, root_size { 0 };

        static bool measure();
};
