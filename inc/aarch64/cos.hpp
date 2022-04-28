/*
 * Class Of Service (COS)
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

#include "status.hpp"
#include "types.hpp"

class Cos final
{
    public:
        /*
         * Determine if a class of service is valid
         *
         * @param cos   Class of service
         * @return      True if valid, false otherwise
         */
        static bool valid_cos (cos_t cos) { return !cos; }

        /*
         * Change the currently active class of service
         */
        static void make_current (cos_t) {}

        static auto cfg_qos (uint8_t)                { return Status::BAD_FTR; }
        static auto cfg_l3_mask (uint16_t, uint32_t) { return Status::BAD_FTR; }
        static auto cfg_l2_mask (uint16_t, uint32_t) { return Status::BAD_FTR; }
        static auto cfg_mb_thrt (uint16_t, uint16_t) { return Status::BAD_FTR; }
};
