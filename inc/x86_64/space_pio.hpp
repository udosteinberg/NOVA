/*
 * PIO Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "bitmap_pio.hpp"
#include "space_hst.hpp"

class Space_pio final : public Space
{
    private:
        Refptr<Space_hst> const hst;
        Bitmap_pio *      const bmp;

        static Space_pio nova;

        Space_pio();

        Space_pio (Refptr<Pd> &p, Refptr<Space_hst> &h, Bitmap_pio *b) : Space { Kobject::Subtype::PIO, p }, hst { std::move (h) }, bmp { b }
        {
            if (hst)
                hst->update (MMAP_SPC_PIO, Kmem::ptr_to_phys (bmp), 1, Paging::R, Memattr::ram());
        }

        ~Space_pio()
        {
            if (hst)
                hst->update (MMAP_SPC_PIO, 0, 1, Paging::NONE, Memattr::ram());

            delete bmp;
        }

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: PIO %p collected", static_cast<void *>(this));
        }

        [[nodiscard]] Paging::Permissions lookup (unsigned long) const;

        void update (unsigned long, Paging::Permissions);

    public:
        static constexpr uint8_t mco { 16 };
        static constexpr uint8_t sbw { 16 };

        [[nodiscard]] Status delegate (Space_pio const *, unsigned long, unsigned long, unsigned, unsigned);

        [[nodiscard]] auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        [[nodiscard]] static Space_pio *create (Status &s, Slab_cache &cache, Pd *pd, bool a)
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

                    auto const pio { new (cache) Space_pio { ref_pd, ref_hst, bmp } };

                    // If we created pio, then references must have been consumed
                    assert (!pio || (!ref_pd && !ref_hst));

                    if (pio) [[likely]]
                        return pio;

                    delete bmp;
                }

                s = Status::MEM_OBJ;
            }

            return nullptr;
        }

        void destroy()
        {
            auto &cache { get_pd()->pio_cache };

            this->~Space_pio();

            operator delete (this, cache);
        }

        static void access_ctrl (uint64_t base, size_t size, Paging::Permissions perm)
        {
            for (unsigned i { 0 }; i < size; i++)       // FIXME: Optimize
                nova.update (base + i, perm);
        }
};
