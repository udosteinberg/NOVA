/*
 * MSR Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "bitmap_msr.hpp"
#include "kmem.hpp"
#include "msr.hpp"
#include "paging.hpp"
#include "space.hpp"

class Space_msr final : public Space
{
    private:
        static constexpr Msr::Reg64 rw[]
        {
            Msr::Reg64::IA32_SYSENTER_CS,
            Msr::Reg64::IA32_SYSENTER_ESP,
            Msr::Reg64::IA32_SYSENTER_EIP,
            Msr::Reg64::IA32_PAT,
            Msr::Reg64::IA32_EFER,
            Msr::Reg64::IA32_FS_BASE,
            Msr::Reg64::IA32_GS_BASE,
            Msr::Reg64::IA32_KERNEL_GS_BASE,
        };

        static constexpr Msr::Reg64 wo[]
        {
            Msr::Reg64::IA32_PRED_CMD,
            Msr::Reg64::IA32_FLUSH_CMD,
        };

        static constexpr Msr::Reg64 ro[]
        {
            Msr::Reg64::IA32_TSC,
            Msr::Reg64::IA32_PLATFORM_ID,
            Msr::Reg64::IA32_BIOS_SIGN_ID,
            Msr::Reg64::IA32_CORE_CAPABILITIES,
            Msr::Reg64::IA32_MPERF,
            Msr::Reg64::IA32_APERF,
            Msr::Reg64::IA32_MTRR_CAP,
            Msr::Reg64::IA32_ARCH_CAPABILITIES,
            Msr::Reg64::IA32_MCU_OPT_CTRL,
            Msr::Reg64::IA32_OVERCLOCKING_STATUS,
            Msr::Reg64::IA32_PERF_STATUS,
            Msr::Reg64::IA32_THERM_STATUS,
            Msr::Reg64::IA32_PACKAGE_THERM_STATUS,
            Msr::Reg64::IA32_XSS,
            Msr::Reg64::IA32_STAR,
            Msr::Reg64::IA32_LSTAR,
            Msr::Reg64::IA32_FMASK,
            Msr::Reg64::IA32_TSC_AUX,
        };

        Bitmap_msr *const bmp;

        static Space_msr nova;

        Space_msr();

        Space_msr (Refptr<Pd> &p, Bitmap_msr *b) : Space { Kobject::Subtype::MSR, p }, bmp { b } {}

        ~Space_msr() { delete bmp; }

        [[nodiscard]] Paging::Permissions lookup (unsigned long) const;

        void update (unsigned long, Paging::Permissions);

    public:
        [[nodiscard]] Status delegate (Space_msr const *, unsigned long, unsigned long, unsigned, unsigned);

        [[nodiscard]] auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        [[nodiscard]] static Space_msr *create (Status &s, Slab_cache &cache, Pd *pd)
        {
            // Acquire reference
            Refptr<Pd> ref_pd { pd };

            // Failed to acquire reference
            if (EXPECT_FALSE (!ref_pd))
                s = Status::ABORTED;

            else {

                auto const bmp { new Bitmap_msr };

                if (EXPECT_TRUE (bmp)) {

                    auto const msr { new (cache) Space_msr { ref_pd, bmp } };

                    // If we created msr, then reference must have been consumed
                    assert (!msr || !ref_pd);

                    if (EXPECT_TRUE (msr))
                        return msr;

                    delete bmp;
                }

                s = Status::MEM_OBJ;
            }

            return nullptr;
        }

        void destroy()
        {
            auto &cache { get_pd()->msr_cache };

            this->~Space_msr();

            operator delete (this, cache);
        }

        static void user_access (Msr::Reg64 r, Paging::Permissions p) { nova.update (std::to_underlying (r), p); }
};
