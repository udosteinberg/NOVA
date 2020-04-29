/*
 * Type Definitions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

using uint8     = unsigned char;
using uint16    = unsigned short;
using uint32    = unsigned int;
using uint64    = unsigned long long;

using mword     = unsigned long;
using Paddr     = unsigned long;

// Signed Integer Types
using int32_t   = __INT32_TYPE__;
using int64_t   = __INT64_TYPE__;

// Unsigned Integer Types
using uint8_t   = __UINT8_TYPE__;
using uint16_t  = __UINT16_TYPE__;
using uint32_t  = __UINT32_TYPE__;
using uint64_t  = __UINT64_TYPE__;
using uint128_t = __uint128_t;

// Pointer Types
using uintptr_t = __UINTPTR_TYPE__;

// Size Types
using size_t    = __SIZE_TYPE__;

// NOVA Types
using apic_t    = uint32_t;
using pci_t     = uint32_t;
using cos_t     = uint16_t;
using cpu_t     = uint16_t;
using port_t    = uint16_t;
