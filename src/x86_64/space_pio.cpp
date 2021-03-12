/*
 * PIO Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

INIT_PRIORITY (PRIO_SPACE_PIO) ALIGNED (Kobject::alignment) Space_pio Space_pio::nova;

/*
 * Constructor (NOVA PIO Space)
 * FIXME: Bitmap allocation failure
 */
Space_pio::Space_pio() : Space { Kobject::Subtype::PIO }, hst { nullptr }, bmp { new Bitmap_pio }
{
    Space_obj::nova.insert (Space_obj::Selector::NOVA_PIO, Capability { this, std::to_underlying (Capability::Perm_sp::TAKE) });

    access_ctrl (0, BIT (16), Paging::R);
}

Space_pio *Space_pio::create (Status &s, Pd *pd, bool a)
{
    // Acquire references
    Refptr<Pd> ref_pd { pd };
    Refptr<Space_hst> ref_hst { a ? pd->get_hst() : nullptr };

    // Failed to acquire references
    if (!ref_pd || (a && !ref_hst)) [[unlikely]]
        s = Status::ABORTED;

    else {

        auto const bmp { new Bitmap_pio };

        if (bmp) [[likely]] {

            // Create new PIO object
            auto const obj { new (ref_pd->pio_cache) Space_pio { ref_pd, ref_hst, bmp } };

            // If creation succeeded, then references must have been consumed
            if (obj) [[likely]] {
                assert (!ref_pd && !ref_hst);
                return obj;
            }

            delete bmp;
        }

        // Failed to create PIO object
        s = Status::MEM_OBJ;
    }

    return nullptr;
}

void Space_pio::destroy()
{
    auto &cache { get_pd()->pio_cache };

    this->~Space_pio();

    operator delete (this, cache);
}

/*
 * Lookup PIO permissions for the specified selector
 *
 * @param s     Selector whose permissions are being looked up
 * @return      Permissions for the specified selector
 */
Paging::Permissions Space_pio::lookup (size_t s) const
{
    assert (bmp->valid (s));

    return Paging::Permissions (!bmp->tst (s) * Paging::R);
}

/*
 * Update PIO permissions for the specified selector
 *
 * @param s     Selector whose permissions are being updated
 * @param p     Permissions for the specified selector
 */
void Space_pio::update (size_t s, Paging::Permissions p)
{
    assert (bmp->valid (s));

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
Status Space_pio::delegate (Space_pio const *pio, size_t ssb, size_t dsb, unsigned ord, unsigned pmm)
{
    auto const e { ssb + BITN (ord) };

    if (ord > sbw || ssb != dsb || !bmp->valid (ssb) || !bmp->valid (e - 1)) [[unlikely]]
        return Status::BAD_PAR;

    for (auto s { ssb }; s < e; s++)
        update (s, Paging::Permissions (pio->lookup (s) & pmm));

    return Status::SUCCESS;
}
