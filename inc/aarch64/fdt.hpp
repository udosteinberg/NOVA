/*
 * Flattened Devicetree (FDT)
 *
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

#include "byteorder.hpp"

class Fdt final
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

        class Header
        {
            private:
                be_uint32_t magic;
                be_uint32_t fdt_size;
                be_uint32_t offs_structs;
                be_uint32_t offs_strings;
                be_uint32_t offs_memmap;
                be_uint32_t fdt_version;
                be_uint32_t last_comp_version;
                be_uint32_t boot_cpu;
                be_uint32_t size_strings;
                be_uint32_t size_structs;

            public:
                uint32_t get_magic()        const { return magic; }
                uint32_t get_fdt_size()     const { return fdt_size; }
                uint32_t get_offs_structs() const { return offs_structs; }
                uint32_t get_offs_strings() const { return offs_strings; }
                uint32_t get_fdt_version()  const { return fdt_version; }
                uint32_t get_boot_cpu()     const { return boot_cpu; }
                uint32_t get_size_structs() const { return size_structs; }

                bool parse (uint64_t) const;
                void parse_subtree (be_uint32_t const *&, unsigned, unsigned, unsigned) const;
        };

        static inline be_uint32_t const *fdtb { nullptr };
        static inline be_uint32_t const *fdte { nullptr };
        static inline char        const *fdts { nullptr };

    public:
        static bool init();
};
