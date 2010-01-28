/*
 * IA32 Paging Support
 *
 * Copyright (C) 2005-2009, Udo Steinberg <udo@hypervisor.org>
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

class Paging
{
    public:
        enum Error
        {
            ERROR_PRESENT           = 1u << 0,      // 0x1
            ERROR_WRITE             = 1u << 1,      // 0x2
            ERROR_USER              = 1u << 2,      // 0x4
            ERROR_RESERVED          = 1u << 3,      // 0x8
            ERROR_IFETCH            = 1u << 4       // 0x10
        };

        enum Attribute
        {
            ATTR_NONE               = 0,
            ATTR_PRESENT            = 1ull << 0,    // 0x1
            ATTR_WRITABLE           = 1ull << 1,    // 0x2
            ATTR_USER               = 1ull << 2,    // 0x4
            ATTR_WRITE_THROUGH      = 1ull << 3,    // 0x8
            ATTR_UNCACHEABLE        = 1ull << 4,    // 0x10
            ATTR_ACCESSED           = 1ull << 5,    // 0x20
            ATTR_DIRTY              = 1ull << 6,    // 0x40
            ATTR_SUPERPAGE          = 1ull << 7,    // 0x80
            ATTR_GLOBAL             = 1ull << 8,    // 0x100
            ATTR_SPLINTER           = 1ull << 9,    // 0x200
            ATTR_NOEXEC             = 0,

            ATTR_PTAB               = ATTR_ACCESSED |
                                      ATTR_USER     |
                                      ATTR_WRITABLE |
                                      ATTR_PRESENT,

            ATTR_LEAF               = ATTR_DIRTY    |
                                      ATTR_ACCESSED |
                                      ATTR_PRESENT,

            ATTR_INVERTED           = ATTR_NOEXEC,

            ATTR_ALL                = ATTR_NOEXEC | ((1ull << 12) - 1)
        };

        static unsigned const levels = 2;
        static unsigned const bpl = 10;

        INIT
        static void enable();

    protected:
        Paddr val;

        ALWAYS_INLINE
        inline Paddr addr() const { return val & ~ATTR_ALL; }

        ALWAYS_INLINE
        inline Attribute attr() const { return Attribute (val & ATTR_ALL); }

        ALWAYS_INLINE
        inline bool present() const { return val & ATTR_PRESENT; }

        ALWAYS_INLINE
        inline bool accessed() const { return val & ATTR_PRESENT; }

        ALWAYS_INLINE
        inline bool dirty() const { return val & ATTR_PRESENT; }

        ALWAYS_INLINE
        inline bool super() const { return val & ATTR_SUPERPAGE; }

        ALWAYS_INLINE
        inline bool global() const { return val & ATTR_GLOBAL; }

        ALWAYS_INLINE
        inline bool splinter() const { return val & ATTR_SPLINTER; }
};
