/*
 * System Memory Management Unit
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

#include "hip.hpp"
#include "intid.hpp"
#include "list.hpp"
#include "mmio.hpp"
#include "sdid.hpp"
#include "status.hpp"

class Dc;
class Space_dma;

class Smmu : public List<Smmu>, protected Mmio
{
    private:
        static inline constinit Smmu *list { nullptr };

        virtual bool init() = 0;
        virtual void fault (unsigned) = 0;
        virtual bool tlb_invalidate (Sdid) = 0;
        virtual bool is_using_iid (unsigned) const = 0;
        virtual uint8_t num_smg() const = 0;
        virtual uint8_t num_ctx() const = 0;

    protected:
        // Timeout for SMMU hardware operations
        static constexpr unsigned timeout { 1 };

        explicit Smmu (uint64_t p, size_t s) : List { list }, Mmio { p, s, Memattr::dev() } {}

    public:
        virtual Status assign_dev (Dc const *, Space_dma *) = 0;

        // FIXME: Reports first SMMU only
        static uint8_t avail_smg() { return list ? list->num_smg() : 0; }
        static uint8_t avail_ctx() { return list ? list->num_ctx() : 0; }

        /*
         * Initialize all enumerated SMMUs
         */
        [[nodiscard]] static bool initialize()
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (!smmu->init()) [[unlikely]]
                    return false;

            if (list) [[likely]]
                Hip::set_feature (Hip_arch::Feature::SMMU);

            return true;
        }

        static Smmu *lookup (uint64_t p)
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (smmu->phys == p)
                    return smmu;

            return nullptr;
        }

        static bool using_iid (unsigned iid)
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (smmu->is_using_iid (iid))
                    return true;

            return false;
        }

        static void interrupt (unsigned iid)
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (smmu->is_using_iid (iid))
                    smmu->fault (iid);
        }

        static void tlb_invalidate_all (Sdid s)
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                smmu->tlb_invalidate (s);
        }
};
