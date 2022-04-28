/*
 * Class Of Service (COS)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
        static inline bool valid_cos (uint16 cos) { return !cos; }

        /*
         * Change the currently active class of service
         */
        static inline void make_current (uint16) {}

        static inline auto cfg_qos (uint8)              { return Status::BAD_FTR; }
        static inline auto cfg_l3_mask (uint16, uint32) { return Status::BAD_FTR; }
        static inline auto cfg_l2_mask (uint16, uint32) { return Status::BAD_FTR; }
        static inline auto cfg_mb_thrt (uint16, uint16) { return Status::BAD_FTR; }
};
