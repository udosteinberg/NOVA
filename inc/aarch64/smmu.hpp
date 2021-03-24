/*
 * System Memory Management Unit (Abstraction Layer)
 *
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

#include "cmdline.hpp"
#include "hip.hpp"
#include "intid.hpp"
#include "list.hpp"
#include "lock_guard.hpp"
#include "mmio.hpp"
#include "status.hpp"
#include "wait.hpp"

class Dc_state;
class Space_dma;

class Smmu : public List<Smmu>, protected Mmio
{
    private:
        virtual void fault (unsigned) = 0;
        [[nodiscard]] virtual bool init() = 0;
        [[nodiscard]] virtual bool invalidate_tlb (uint16_t) = 0;
        [[nodiscard]] virtual bool is_using_iid (unsigned) const = 0;
        virtual uint8_t num_smg() const = 0;
        virtual uint8_t num_ctx() const = 0;

    protected:
        // Timeout for SMMU operations (in ms)
        static constexpr unsigned timeout { 5 };

        // List of SMMUs
        static inline constinit Smmu *list {};

        [[nodiscard]] explicit Smmu (uint64_t p, size_t s) : Mmio { p, s, Memattr::dev() } {}

    public:
        // Public interface
        [[nodiscard]] virtual Status assign_dev (Dc_state const *, Space_dma *, Space_dma *, uintptr_t &) = 0;

        // FIXME: Reports first SMMU only
        static uint8_t avail_smg() { return list ? list->num_smg() : 0; }
        static uint8_t avail_ctx() { return list ? list->num_ctx() : 0; }

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

        /*
         * Invalidate TLB entry in all SMMUs
         */
        [[nodiscard]] static bool all_invalidate_tlb (uint16_t dom)
        {
            bool ret { true };

            for (auto smmu { list }; smmu; smmu = smmu->next)
                ret &= smmu->invalidate_tlb (dom);

            return ret;
        }

        /*
         * Lookup SMMU based on physical address
         */
        [[nodiscard]] static Smmu *lookup_phys (uint64_t phys)
        {
            for (auto smmu { list }; smmu; smmu = smmu->next)
                if (smmu->phys == phys)
                    return smmu;

            return nullptr;
        }

        /*
         * Initialize all SMMUs
         */
        [[nodiscard]] static bool initialize()
        {
            bool ret { true };

            if (!Cmdline::nosmmu) [[likely]] {

                for (auto smmu { list }; smmu; smmu = smmu->next)
                    ret &= smmu->init();

                if (list && ret) [[likely]]
                    Hip::set_feature (Hip_arch::Feature::SMMU_DMA);
            }

            return ret;
        }
};
