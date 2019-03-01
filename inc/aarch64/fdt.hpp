/*
 * Flattened Devicetree (FDT)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

class Fdt
{
    private:
        enum
        {
            FDT_BEGIN_NODE  = 0x1,
            FDT_END_NODE    = 0x2,
            FDT_PROP        = 0x3,
            FDT_NOP         = 0x4,
            FDT_END         = 0x9,
        };

        uint32  __magic;
        uint32  __fdt_size;
        uint32  __offs_structs;
        uint32  __offs_strings;
        uint32  __offs_memmap;
        uint32  __fdt_version;
        uint32  __last_comp_version;
        uint32  __boot_cpu;
        uint32  __size_strings;
        uint32  __size_structs;

        static uint32 const *fdtb;
        static uint32 const *fdte;
        static char   const *fdts;

        static uint32 convert (uint32 x) { return __builtin_bswap32 (x); }

        bool    valid()         const { return convert (__magic) == 0xd00dfeed; }
        uint32  fdt_size()      const { return convert (__fdt_size); }
        uint32  fdt_version()   const { return convert (__fdt_version); }
        uint32  boot_cpu()      const { return convert (__boot_cpu); }
        uint32  offs_structs()  const { return convert (__offs_structs); }
        uint32  offs_strings()  const { return convert (__offs_strings); }
        uint32  size_structs()  const { return convert (__size_structs); }

        void parse (uint64) const;
        void parse_subtree (uint32 const *&, unsigned, unsigned, unsigned) const;

    public:
        static void init();
};
