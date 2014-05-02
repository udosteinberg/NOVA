/*
 * Hypervisor Information Page (HIP)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
#include "config.hpp"
#include "extern.hpp"

class Hip_cpu
{
    public:
        uint8   flags;
        uint8   thread;
        uint8   core;
        uint8   package;
        uint32  reserved;
};

class Hip_mem
{
    public:
        enum {
            HYPERVISOR  = -1u,
            MB_MODULE   = -2u
        };

        uint64  addr;
        uint64  size;
        uint32  type;
        uint32  aux;
};

class Hip
{
    private:
        uint32  signature;              // 0x0
        uint16  checksum;               // 0x4
        uint16  length;                 // 0x6
        uint16  cpu_offs;               // 0x8
        uint16  cpu_size;               // 0xa
        uint16  mem_offs;               // 0xc
        uint16  mem_size;               // 0xe
        uint32  api_flg;                // 0x10
        uint32  api_ver;                // 0x14
        uint32  sel_num;                // 0x18
        uint32  sel_exc;                // 0x1c
        uint32  sel_vmi;                // 0x20
        uint32  sel_gsi;                // 0x24
        uint32  cfg_page;               // 0x28
        uint32  cfg_utcb;               // 0x2c
        uint32  freq_tsc;               // 0x30
        uint32  reserved;               // 0x34
        Hip_cpu cpu_desc[NUM_CPU];
        Hip_mem mem_desc[];

    public:
        enum Feature {
            FEAT_VMX    = 1u << 1,
            FEAT_SVM    = 1u << 2,
        };

        static mword root_addr;
        static mword root_size;

        ALWAYS_INLINE
        static inline Hip *hip()
        {
            return reinterpret_cast<Hip *>(&PAGE_H);
        }

        static uint32 feature()
        {
            return hip()->api_flg;
        }

        static void remove (Feature f)
        {
            Atomic::clr_mask (hip()->api_flg, static_cast<typeof hip()->api_flg>(f));
        }

        static bool cpu_online (unsigned long cpu)
        {
            return cpu < NUM_CPU && hip()->cpu_desc[cpu].flags & 1;
        }

        INIT
        static void build (mword);

        INIT
        static void add_mem (Hip_mem *&, mword, size_t);

        INIT
        static void add_mod (Hip_mem *&, mword, size_t);

        INIT
        static void add_mhv (Hip_mem *&);

        static void add_cpu();
        static void add_check();
};
