/*
 * Guest Memory Space
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "ptab_npt.hpp"
#include "space_mem.hpp"

class Space_gst final : public Space_mem<Space_gst>
{
    private:
        Vmid const  vmid;
        Nptp        nptp;

        Space_gst (Refptr<Pd> &p) : Space_mem { Kobject::Subtype::GST, p } {}

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: GST %p collected", static_cast<void *>(this));
        }

    public:
        static constexpr uint8_t  sbw       { Npt::ibits - PAGE_BITS };
        static constexpr uint64_t selectors { BIT64 (sbw) };

        static auto mco() { return static_cast<uint8_t>(Npt::lev_ord()); }

        [[nodiscard]] static Space_gst *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            // Acquire reference
            Refptr<Pd> ref_pd { pd };

            // Failed to acquire reference
            if (!ref_pd) [[unlikely]]
                s = Status::ABORTED;

            else {

                auto const gst { new (cache) Space_gst { ref_pd } };

                // If we created gst, then reference must have been consumed
                assert (!gst || !ref_pd);

                if (gst) [[likely]] {

                    if (gst->nptp.root_init()) [[likely]]
                        return gst;

                    operator delete (gst, cache);
                }

                s = Status::MEM_OBJ;
            }

            return nullptr;
        }

        void destroy()
        {
            auto &cache { get_pd()->gst_cache };

            this->~Space_gst();

            operator delete (this, cache);
        }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma) { return nptp.update (v, p, o, pm, ma); }

        void sync() { nptp.invalidate (vmid); }

        void make_current() { nptp.make_current (vmid); }
};
