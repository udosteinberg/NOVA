/*
 * Type Support
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
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

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

typedef unsigned long       mword;
typedef unsigned long       Paddr;

typedef __INT32_TYPE__      int32_t;
typedef __INT64_TYPE__      int64_t;

typedef __SIZE_TYPE__       size_t;
typedef __UINTPTR_TYPE__    uintptr_t;

/*
 * Chooses type F (if false) or type T (if true) based on a compile-time boolean
 */
template<bool, class F, class T> struct conditional {};
template<      class F, class T> struct conditional<false, F, T> { typedef F type; };
template<      class F, class T> struct conditional<true,  F, T> { typedef T type; };

enum class Uart_type { NONE, CDNS, GENI, IMX, MESON, MINI, NS16550, PL011, SCIF, };
