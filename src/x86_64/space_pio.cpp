/*
 * PIO Space
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

#include "space_obj.hpp"
#include "space_pio.hpp"

INIT_PRIORITY (PRIO_SPACE_PIO)
ALIGNED (Kobject::alignment) Space_pio Space_pio::nova;

/*
 * Constructor (NOVA PIO Space)
 */
Space_pio::Space_pio() : bmp (new Bitmap_pio)
{
    // FIXME: Bitmap allocation failure
    user_access (0, BIT (16), true);
}

/*
 * Lookup PIO permissions for the specified selector
 *
 * @param s     Selector whose permissions are being looked up
 * @return      Permissions for the specified selector
 */
Paging::Permissions Space_pio::lookup (unsigned long s) const
{
    if (EXPECT_TRUE (bmp->sel_valid (s)))
        return Paging::Permissions (!bmp->tst (s) * Paging::R);

    return Paging::NONE;
}

/*
 * Update PIO permissions for the specified selector
 *
 * @param s     Selector whose permissions are being updated
 * @param p     Permissions for the specified selector
 */
void Space_pio::update (unsigned long s, Paging::Permissions p)
{
    if (EXPECT_TRUE (bmp->sel_valid (s)))
        p & Paging::R ? bmp->clr (s) : bmp->set (s);
}

/*
 * Delegate PIO capability range
 *
 * @param pio   SRC PIO space
 * @param ssb   SRC selector base
 * @param dsb   DST selector base
 * @param ord   Order (2^ord selectors)
 * @param pmm   Permission mask
 * @return      SUCCESS (successful) or BAD_PAR (bad parameter)
 */
Status Space_pio::delegate (Space_pio const *pio, unsigned long ssb, unsigned long dsb, unsigned ord, unsigned pmm)
{
    auto const e { ssb + BITN (ord) };

    if (EXPECT_FALSE (ssb != dsb || !Bitmap_pio::sel_valid (e - 1)))
        return Status::BAD_PAR;

    for (auto s { ssb }; s < e; s++)
        update (s, Paging::Permissions (pio->lookup (s) & pmm));

    return Status::SUCCESS;
}
