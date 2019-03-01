/*
 * Flattened Devicetree (FDT)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

        struct Header
        {
            Unaligned_be<uint32_t> const magic;
            Unaligned_be<uint32_t> const fdt_size;
            Unaligned_be<uint32_t> const offs_structs;
            Unaligned_be<uint32_t> const offs_strings;
            Unaligned_be<uint32_t> const offs_memmap;
            Unaligned_be<uint32_t> const fdt_version;
            Unaligned_be<uint32_t> const last_comp_version;
            Unaligned_be<uint32_t> const boot_cpu;
            Unaligned_be<uint32_t> const size_strings;
            Unaligned_be<uint32_t> const size_structs;

            bool parse (uint64_t) const;
            void parse_subtree (Unaligned_be<uint32_t> const *&, unsigned, unsigned, unsigned) const;
        };

        static_assert (__is_standard_layout (Header) && alignof (Header) == 1 && sizeof (Header) == 40);

        static inline constinit Unaligned_be<uint32_t> const *fdtb { nullptr };
        static inline constinit Unaligned_be<uint32_t> const *fdte { nullptr };
        static inline constinit char                   const *fdts { nullptr };

    public:
        static bool init();
};
