/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "compiler.h"
#include "types.h"

#define SIG(A,B,C,D) (A + (B << 8) + (C << 16) + (D << 24))

/*
 * Root System Description Pointer (5.2.5)
 */
class Acpi_rsdp
{
    private:
        uint32  signature[2];
        uint8   checksum;
        char    oem_id[6];
        uint8   revision;
        uint32  rsdt_addr;
        uint32  length;
        uint64  xsdt_addr;
        uint8   extended_checksum;

        ALWAYS_INLINE INIT
        bool good_signature() const
        {
            return signature[0] == SIG ('R','S','D',' ') &&
                   signature[1] == SIG ('P','T','R',' ');
        }

        ALWAYS_INLINE INIT
        bool good_checksum (unsigned len = 20) const
        {
            uint8 check = 0;
            for (uint8 const *ptr = reinterpret_cast<uint8 const *>(this);
                              ptr < reinterpret_cast<uint8 const *>(this) + len;
                              check = static_cast<uint8>(check + *ptr++)) ;
            return !check;
        }

        INIT
        static Acpi_rsdp *find (mword start, unsigned len);

    public:
        INIT
        static void parse();
};
