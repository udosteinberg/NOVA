/*
 * MSR Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "space_msr.hpp"
#include "space_obj.hpp"

INIT_PRIORITY (PRIO_SPACE_MSR) ALIGNED (Kobject::alignment) Space_msr Space_msr::nova;

/*
 * Constructor (NOVA MSR Space)
 */
Space_msr::Space_msr() : bmp (new Bitmap_msr)
{
    // FIXME: Bitmap allocation failure
    // MSR whitelisting here
}

/*
 * Lookup MSR permissions for the specified selector
 *
 * @param s     Selector whose permissions are being looked up
 * @return      Permissions for the specified selector
 */
Paging::Permissions Space_msr::lookup (unsigned long s) const
{
    if (EXPECT_TRUE (bmp->sel_valid (s)))
        return Paging::Permissions (!bmp->tst_w (s) * Paging::W |
                                    !bmp->tst_r (s) * Paging::R);

    return Paging::NONE;
}

/*
 * Update MSR permissions for the specified selector
 *
 * @param s     Selector whose permissions are being updated
 * @param p     Permissions for the specified selector
 */
void Space_msr::update (unsigned long s, Paging::Permissions p)
{
    if (EXPECT_TRUE (bmp->sel_valid (s))) {
        p & Paging::W ? bmp->clr_w (s) : bmp->set_w (s);
        p & Paging::R ? bmp->clr_r (s) : bmp->set_r (s);
    }
}

/*
 * Delegate MSR capability range
 *
 * @param msr   SRC MSR space
 * @param ssb   SRC selector base
 * @param dsb   DST selector base
 * @param ord   Order (2^ord selectors)
 * @param pmm   Permission mask
 * @return      SUCCESS (successful) or BAD_PAR (bad parameter)
 */
Status Space_msr::delegate (Space_msr const *msr, unsigned long ssb, unsigned long dsb, unsigned ord, unsigned pmm)
{
    auto const e { ssb + BITN (ord) };

    if (EXPECT_FALSE (ssb != dsb || !Bitmap_msr::sel_valid (e - 1)))
        return Status::BAD_PAR;

    for (auto s { ssb }; s < e; s++)
        update (s, Paging::Permissions (msr->lookup (s) & pmm));

    return Status::SUCCESS;
}
