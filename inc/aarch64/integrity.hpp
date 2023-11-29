/*
 * Integrity Measurement
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

class Integrity final
{
    private:
        static bool extend (unsigned, Hash_sha1_160 &, Hash_sha2_256 &, Hash_sha2_384 &, Hash_sha2_512 &) { return false; }

    public:
        static inline uint64_t root_phys { 0 }, root_size { 0 };

        static bool measure();
};
