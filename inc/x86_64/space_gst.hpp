/*
 * Guest Memory Space
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

#pragma once

#include "cpuset.hpp"
#include "hip.hpp"
#include "ptab_gst_amd.hpp"
#include "ptab_gst_itl.hpp"
#include "space_mem.hpp"

class Space_gst : public Space_mem<Space_gst>
{
    private:
        Ptab_gst_amd    gst_amd;
        Ptab_gst_itl    gst_itl;

        enum class Type : uint8_t { NONE, AMD, ITL };

        static Type type()
        {
            if (Hip::hip->feature() & Hip::FEAT_VMX)
                return Type::ITL;

            if (Hip::hip->feature() & Hip::FEAT_SVM)
                return Type::AMD;

            return Type::NONE;
        }

        [[nodiscard]] bool root_init()
        {
            switch (type()) {
                case Type::AMD: return gst_amd.root_init();
                case Type::ITL: return gst_itl.root_init();
                default:        return false;
            }
        }

    public:
        Cpuset gtlb;

        [[nodiscard]] auto get_root_amd() const { return gst_amd.root_hw(); }
        [[nodiscard]] auto get_root_itl() const { return gst_itl.root_hw(); }

        static uint8_t sbw()
        {
            switch (type()) {
                case Type::AMD: return Pte_gst_amd::ibits - PAGE_BITS;
                case Type::ITL: return Pte_gst_itl::ibits - PAGE_BITS;
                default: return 0;
            }
        }

        static uint8_t mco()
        {
            switch (type()) {
                case Type::AMD: return static_cast<uint8_t>(Pte_gst_amd::lev_ord());
                case Type::ITL: return static_cast<uint8_t>(Pte_gst_itl::lev_ord());
                default: return 0;
            }
        }

        auto lookup (uint64_t v, uint64_t &p, unsigned &o, Memattr &ma) const
        {
            switch (type()) {
                case Type::AMD: return gst_amd.lookup (v, p, o, ma);
                case Type::ITL: return gst_itl.lookup (v, p, o, ma);
                default:        return Paging::NONE;
            }
        }

        auto update (uint64_t v, uint64_t p, unsigned o, Paging::Permissions pm, Memattr ma, Tlb::Invalidate &inv)
        {
            switch (type()) {
                case Type::AMD: return gst_amd.update (v, p, o, pm, ma, inv);
                case Type::ITL: return gst_itl.update (v, p, o, pm, ma, inv);
                default:        return Status::BAD_FTR;
            }
        }

        [[nodiscard]] bool invalidate (Tlb::Invalidate inv)
        {
            switch (inv) {

                case Tlb::Invalidate::SELF:
                    gtlb.set (Cpu::id);
                    return true;

                default:
                    gtlb.set_all();
                    return true;
            }
        }

        [[nodiscard]] auto invalidate() const { return gst_itl.invalidate(); }
};
