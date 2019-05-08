/*
 * Hypervisor Information Page (HIP)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "types.hpp"

class Hip
{
    private:
        uint32  signature;              // 0
        uint16  checksum;               // 4
        uint16  length;                 // 6
        uint64  nova_s_addr;            // 8
        uint64  nova_e_addr;            // 16
        uint64  mbuf_s_addr;            // 24
        uint64  mbuf_e_addr;            // 32
        uint64  root_s_addr;            // 40
        uint64  root_e_addr;            // 48
        uint64  sel_num;                // 56
        uint16  sel_hst_arch;           // 64
        uint16  sel_hst_nova;           // 66
        uint16  sel_gst_arch;           // 68
        uint16  sel_gst_nova;           // 70
        uint16  cpu_num;                // 72
        uint16  int_num;                // 74
        uint16  smg_num;                // 76
        uint16  ctx_num;                // 78

    public:
        static Hip *hip;

        void build (uint64, uint64);
};
