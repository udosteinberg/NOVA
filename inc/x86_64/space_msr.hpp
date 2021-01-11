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

#pragma once

#include "bitmap_msr.hpp"
#include "kmem.hpp"
#include "msr.hpp"
#include "paging.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_msr : public Space
{
    friend class Pd;

    private:
        static constexpr Msr::Register rw[]
        {
            Msr::Register::IA32_SYSENTER_CS,
            Msr::Register::IA32_SYSENTER_ESP,
            Msr::Register::IA32_SYSENTER_EIP,
            Msr::Register::IA32_PAT,
            Msr::Register::IA32_EFER,
            Msr::Register::IA32_FS_BASE,
            Msr::Register::IA32_GS_BASE,
            Msr::Register::IA32_KERNEL_GS_BASE,
        };

        static constexpr Msr::Register wo[]
        {
            Msr::Register::IA32_PRED_CMD,
            Msr::Register::IA32_FLUSH_CMD,
        };

        static constexpr Msr::Register ro[]
        {
            Msr::Register::IA32_TSC,
            Msr::Register::IA32_PLATFORM_ID,
            Msr::Register::IA32_BIOS_SIGN_ID,
            Msr::Register::IA32_CORE_CAPABILITIES,
            Msr::Register::IA32_MPERF,
            Msr::Register::IA32_APERF,
            Msr::Register::IA32_MTRR_CAP,
            Msr::Register::IA32_ARCH_CAPABILITIES,
            Msr::Register::IA32_OVERCLOCKING_STATUS,
            Msr::Register::IA32_PERF_STATUS,
            Msr::Register::IA32_THERM_STATUS,
            Msr::Register::IA32_PACKAGE_THERM_STATUS,
            Msr::Register::IA32_XSS,
            Msr::Register::IA32_STAR,
            Msr::Register::IA32_LSTAR,
            Msr::Register::IA32_FMASK,
            Msr::Register::IA32_TSC_AUX,
        };

        Bitmap_msr *const bmp;

        static Space_msr nova;

        Space_msr();

        Space_msr (Bitmap_msr *b) : bmp (b) {}

        ~Space_msr() { delete bmp; }

        [[nodiscard]] Paging::Permissions lookup (unsigned long) const;

        void update (unsigned long, Paging::Permissions);

    public:
        [[nodiscard]] Status delegate (Space_msr const *, unsigned long, unsigned long, unsigned, unsigned);

        [[nodiscard]] auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        static void user_access (Msr::Register r, Paging::Permissions p) { nova.update (std::to_underlying (r), p); }
};
