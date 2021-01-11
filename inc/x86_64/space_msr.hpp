/*
 * MSR Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "bitmap_msr.hpp"
#include "paging.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_msr : private Space
{
    private:
        Bitmap_msr * const cpu_gst;

        Paging::Permissions lookup (unsigned long) const;

    protected:
        /*
         * Constructor
         *
         * @param g     Pointer to MSR bitmap for guest (or nullptr)
         */
        inline Space_msr (Bitmap_msr *g) : cpu_gst (g) {}

        /*
         * Destructor
         */
        inline ~Space_msr()
        {
            delete cpu_gst;
        }

        void update (unsigned long, Paging::Permissions, Bitmap_msr *);

        Status delegate (Space_msr const *, unsigned long, unsigned long, unsigned, unsigned, Space::Index);

    public:
        inline auto bmp_gst() const { return cpu_gst; }
};
