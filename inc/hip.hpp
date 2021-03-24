/*
 * Hypervisor Information Page (HIP): Architecture-Independent Part
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#pragma once

#include "atomic.hpp"
#include "hip_arch.hpp"
#include "std.hpp"

class Hip final
{
    private:
        using feat_t = uint64;

        uint32          signature;                                                  // 0x0
        uint16          checksum, length;                                           // 0x4
        uint64          nova_p_addr, nova_e_addr;                                   // 0x8
        uint64          mbuf_p_addr, mbuf_e_addr;                                   // 0x18
        uint64          root_p_addr, root_e_addr;                                   // 0x28
        uint64          acpi_rsdp_addr, uefi_mmap_addr;                             // 0x38
        uint32          uefi_mmap_size;                                             // 0x48
        uint16          uefi_desc_size, uefi_desc_vers;                             // 0x4c
        uint64          tmr_frq, sel_num;                                           // 0x50
        uint16          sel_hst_arch, sel_hst_nova, sel_gst_arch, sel_gst_nova;     // 0x60
        uint16          cpu_num, cpu_bsp, int_pin, int_msi;                         // 0x68
        uint8           mco_obj, mco_hst, mco_gst, mco_dma, mco_pio, mco_msr;       // 0x70
        Atomic<feat_t>  features;                                                   // 0x78
        Hip_arch        arch;                                                       // 0x80

    public:
        static Hip *hip;

        static inline bool feature (Hip_arch::Feature f)
        {
            return hip->features & std::to_underlying (f);
        }

        static inline void set_feature (Hip_arch::Feature f)
        {
            hip->features |= std::to_underlying (f);
        }

        static inline void clr_feature (Hip_arch::Feature f)
        {
            hip->features &= ~std::to_underlying (f);
        }

        void build (uint64, uint64);
};
