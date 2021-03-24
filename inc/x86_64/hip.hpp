/*
 * Hypervisor Information Page (HIP): Architecture-Independent Part
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

class Hip
{
    private:
        typedef uint64  feat_t;

        uint32          signature;          // 0x0
        uint16          checksum;           // 0x4
        uint16          length;             // 0x6
        uint64          nova_p_addr;        // 0x8
        uint64          nova_e_addr;        // 0x10
        uint64          mbuf_p_addr;        // 0x18
        uint64          mbuf_e_addr;        // 0x20
        uint64          root_p_addr;        // 0x28
        uint64          root_e_addr;        // 0x30
        uint64          acpi_rsdp_addr;     // 0x38
        uint64          uefi_mmap_addr;     // 0x40
        uint32          uefi_mmap_size;     // 0x48
        uint16          uefi_desc_size;     // 0x4c
        uint16          uefi_desc_vers;     // 0x4e
        uint64          tmr_frq;            // 0x50
        uint64          sel_num;            // 0x58
        uint16          sel_hst_arch;       // 0x60
        uint16          sel_hst_nova;       // 0x62
        uint16          sel_gst_arch;       // 0x64
        uint16          sel_gst_nova;       // 0x66
        uint16          cpu_num;            // 0x68
        uint16          cpu_bsp;            // 0x6a
        uint16          int_num;            // 0x6c
        uint16          reserved;           // 0x6e
        Atomic<feat_t>  features;           // 0x70
        Hip_arch        arch;               // 0x78

    public:
        static Hip *hip;

        static inline bool feature (Hip_arch::Feature f)
        {
            return hip->features & static_cast<feat_t>(f);
        }

        static inline void set_feature (Hip_arch::Feature f)
        {
            hip->features |= static_cast<feat_t>(f);
        }

        static inline void clr_feature (Hip_arch::Feature f)
        {
            hip->features &= ~static_cast<feat_t>(f);
        }

        void build (uint64, uint64);
};
