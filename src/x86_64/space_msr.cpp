/*
 * MSR Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "space_msr.hpp"
#include "space_obj.hpp"

INIT_PRIORITY (PRIO_SPACE_MSR) ALIGNED (Kobject::alignment) Space_msr Space_msr::nova;

/*
 * Constructor (NOVA MSR Space)
 * FIXME: Bitmap allocation failure
 */
Space_msr::Space_msr() : Space (Kobject::Subtype::MSR, nullptr), bmp (new Bitmap_msr)
{
    Space_obj::nova.insert (Space_obj::Selector::NOVA_MSR, Capability (this, std::to_underlying (Capability::Perm_sp::TAKE)));

    constexpr Msr::Register rw[] { Msr::Register::IA32_SYSENTER_CS,
                                   Msr::Register::IA32_SYSENTER_ESP,
                                   Msr::Register::IA32_SYSENTER_EIP,
                                   Msr::Register::IA32_PAT,
                                   Msr::Register::IA32_EFER,
                                   Msr::Register::IA32_FS_BASE,
                                   Msr::Register::IA32_GS_BASE,
                                   Msr::Register::IA32_KERNEL_GS_BASE };

    constexpr Msr::Register ro[] { Msr::Register::IA32_XSS,
                                   Msr::Register::IA32_STAR,
                                   Msr::Register::IA32_LSTAR,
                                   Msr::Register::IA32_FMASK,
                                   Msr::Register::IA32_TSC_AUX };

    for (auto msr : rw)
        user_access (msr, Paging::Permissions (Paging::W | Paging::R));

    for (auto msr : ro)
        user_access (msr, Paging::Permissions (Paging::R));
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
