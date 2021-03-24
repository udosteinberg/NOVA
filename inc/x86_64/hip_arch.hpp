/*
 * Hypervisor Information Page (HIP): Architecture-Specific Part (x86)
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

#include "macros.hpp"
#include "types.hpp"

class Hip_arch final
{
    private:
        uint64_t    elog_p_addr, elog_e_addr;

    public:
        enum class Feature : uint64_t
        {
            IOMMU   = BIT (0),
            VMX     = BIT (1),
            SVM     = BIT (2),
        };

        void build();
};
