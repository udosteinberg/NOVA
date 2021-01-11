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

#include "space_msr.hpp"

/*
 * Lookup MSR permissions for the specified selector
 *
 * @param s     Selector whose permissions are being looked up
 * @return      Permissions for the specified selector
 */
Paging::Permissions Space_msr::lookup (unsigned long s) const
{
    auto b = cpu_gst;

    if (EXPECT_TRUE (b && b->sel_valid (s)))
        return Paging::Permissions (!b->tst_w (s) * Paging::W |
                                    !b->tst_r (s) * Paging::R);

    return Paging::NONE;
}

/*
 * Update MSR permissions for the specified selector
 *
 * @param s     Selector whose permissions are being updated
 * @param p     Permissions for the specified selector
 * @param b     Pointer to the MSR bitmap that is being updated
 */
void Space_msr::update (unsigned long s, Paging::Permissions p, Bitmap_msr *b)
{
    if (EXPECT_TRUE (b && b->sel_valid (s))) {
        p & Paging::W ? b->clr_w (s) : b->set_w (s);
        p & Paging::R ? b->clr_r (s) : b->set_r (s);
    }
}

/*
 * Delegate MSR capability range
 *
 * @param msr   Source MSR space
 * @param src   Selector base (source)
 * @param dst   Selector base (destination)
 * @param ord   Selector order (2^ord selectors)
 * @param pmm   Permission mask
 * @param si    Space index
 * @return      SUCCESS (successful) or BAD_PAR (bad parameter)
 */
Status Space_msr::delegate (Space_msr const *msr, unsigned long src, unsigned long dst, unsigned ord, unsigned pmm, Space::Index si)
{
    Bitmap_msr *bmp;

    switch (si) {
        default: return Status::BAD_PAR;
        case Space::Index::CPU_GST: bmp = cpu_gst; break;
    }

    auto const e = src + BITN (ord);

    if (EXPECT_FALSE (src != dst || !Bitmap_msr::sel_valid (e - 1)))
        return Status::BAD_PAR;

    for (auto s = src; s < e; s++)
        update (s, Paging::Permissions (msr->lookup (s) & pmm), bmp);

    return Status::SUCCESS;
}
