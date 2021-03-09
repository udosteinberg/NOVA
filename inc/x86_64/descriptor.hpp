/*
 * Descriptor
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "compiler.hpp"
#include "macros.hpp"
#include "std.hpp"

/*
 * Descriptor Base Class
 */
template<unsigned N> struct Descriptor
{
    uint32_t val[N];

    enum class Type : unsigned
    {
        // System Segments
        SYS_LDT                 = 0x2,
        SYS_TSS                 = 0x9,
        SYS_CALL_GATE           = 0xc,
        SYS_INTR_GATE           = 0xe,
        SYS_TRAP_GATE           = 0xf,

        // Data Segments
        DATA_R                  = 0x10,
        DATA_RA                 = 0x11,
        DATA_RW                 = 0x12,
        DATA_RWA                = 0x13,
        DATA_DOWN_R             = 0x14,
        DATA_DOWN_RA            = 0x15,
        DATA_DOWN_RW            = 0x16,
        DATA_DOWN_RWA           = 0x17,

        // Code Segments
        CODE_X                  = 0x18,
        CODE_XA                 = 0x19,
        CODE_XR                 = 0x1a,
        CODE_XRA                = 0x1b,
        CODE_CONF_X             = 0x1c,
        CODE_CONF_XA            = 0x1d,
        CODE_CONF_XR            = 0x1e,
        CODE_CONF_XRA           = 0x1f,
    };
};

/*
 * Descriptor: Page-Granular 64-Bit Code/Data Segment
 */
struct Descriptor_gdt_seg final : public Descriptor<2>
{
    explicit constexpr Descriptor_gdt_seg() : Descriptor { 0, 0 } {}

    /*
     * Constructor
     *
     * @param t Descriptor Type
     * @param d Descriptor Privilege Level (0...3)
     */
    explicit constexpr Descriptor_gdt_seg (Type t, unsigned d) : Descriptor { 0, BIT (23) | BIT (21) | BIT (15) | d << 13 | std::to_underlying (t) << 8 } {}
};

static_assert (__is_standard_layout (Descriptor_gdt_seg) && sizeof (Descriptor_gdt_seg) == 8);

/*
 * Descriptor: Byte-Granular 64-Bit System Segment
 */
struct Descriptor_gdt_sys final : public Descriptor<4>
{
    explicit constexpr Descriptor_gdt_sys() : Descriptor { 0, 0, 0, 0 } {}

    /*
     * Constructor
     *
     * @param t Descriptor Type
     * @param b Segment Base Address
     * @param l Segment Limit
     */
    explicit constexpr Descriptor_gdt_sys (Type t, uint64_t b, uint32_t l) : Descriptor { static_cast<uint32_t>(b << 16 | (l & BIT_RANGE (15, 0))),
                                                                                          static_cast<uint32_t>((b & BIT_RANGE (31, 24)) | (l & BIT_RANGE (19, 16)) | BIT (15) | std::to_underlying (t) << 8 | (b >> 16 & BIT_RANGE (7, 0))),
                                                                                          static_cast<uint32_t>(b >> 32), 0 } {}
};

static_assert (__is_standard_layout (Descriptor_gdt_sys) && sizeof (Descriptor_gdt_sys) == 16);

/*
 * Descriptor: 64-Bit IDT Gate
 */
struct Descriptor_idt final : public Descriptor<4>
{
    explicit constexpr Descriptor_idt() : Descriptor { 0, 0, 0, 0 } {}

    /*
     * Constructor
     *
     * @param d Descriptor Privilege Level (0...3)
     * @param i Interrupt Stack Table Slot (0...7)
     * @param s Segment Selector (Destination Code Segment)
     * @param e Entry Instruction Pointer (Interrupt Handler)
     */
    explicit constexpr Descriptor_idt (unsigned d, unsigned i, uint16_t s, uint64_t e) : Descriptor { static_cast<uint32_t>(s << 16 | (e & BIT_RANGE (15, 0))),
                                                                                                      static_cast<uint32_t>((e & BIT_RANGE (31, 16)) | BIT (15) | d << 13 | std::to_underlying (Type::SYS_INTR_GATE) << 8 | i),
                                                                                                      static_cast<uint32_t>(e >> 32), 0 } {}
};

static_assert (__is_standard_layout (Descriptor_idt) && sizeof (Descriptor_idt) == 16);

/*
 * Pseudo Descriptor for LGDT/LIDT
 */
class Pseudo_descriptor final
{
    private:
        Unaligned_le<uint16_t>      limit;
        Unaligned_le<uintptr_t>     base;

    public:
        explicit Pseudo_descriptor (void *b, size_t l) : limit { static_cast<uint16_t>(l - 1) }, base { reinterpret_cast<uintptr_t>(b) } {}
};

static_assert (__is_standard_layout (Pseudo_descriptor) && alignof (Pseudo_descriptor) == 1 && sizeof (Pseudo_descriptor) == 10);
