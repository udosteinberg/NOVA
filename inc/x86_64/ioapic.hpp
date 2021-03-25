/*
 * I/O Advanced Programmable Interrupt Controller (IOAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "list.hpp"
#include "lock_guard.hpp"
#include "macros.hpp"
#include "slab.hpp"

class Ioapic : public List<Ioapic>
{
    private:
        uintptr_t   const       reg_base    { 0 };
        unsigned    const       gsi_base    { 0 };
        unsigned    const       id          { 0 };
        uint16                  rid         { 0 };
        Spinlock                lock;

        static inline   Ioapic *    list    { nullptr };
        static          Slab_cache  cache;

        // Direct Registers
        enum class Dir_Register8
        {
            IND     = 0x0,                  // Index Register
        };

        // Direct Registers
        enum class Dir_Register32
        {
            DAT     = 0x10,                 // Data Register
            PAR     = 0x20,                 // Pin Assertion Register
            EOI     = 0x40,                 // EOI Register
        };

        // Indirect Registers
        enum class Ind_Register32
        {
            ID      = 0x0,                  // ID Register
            VER     = 0x1,                  // Version Register
            ARB     = 0x2,                  // Arbitration ID Register
            BCFG    = 0x3,                  // Boot Configuration Register
            RTE     = 0x10,                 // Redirection Table Entry Register
        };

        inline auto read  (Dir_Register32 r)    { return *reinterpret_cast<uint32 volatile *>(reg_base + static_cast<unsigned>(r)); }
        inline void write (Dir_Register32 r, uint32 v) { *reinterpret_cast<uint32 volatile *>(reg_base + static_cast<unsigned>(r)) = v; }
        inline void write (Dir_Register8  r, uint8  v) { *reinterpret_cast<uint8  volatile *>(reg_base + static_cast<unsigned>(r)) = v; }

        ALWAYS_INLINE
        inline void index (Ind_Register32 r)
        {
            write (Dir_Register8::IND, static_cast<uint8>(r));
        }

        ALWAYS_INLINE
        inline uint32 read (Ind_Register32 r)
        {
            Lock_guard <Spinlock> guard (lock);
            index (r);
            return read (Dir_Register32::DAT);
        }

        ALWAYS_INLINE
        inline void write (Ind_Register32 r, uint32 v)
        {
            Lock_guard <Spinlock> guard (lock);
            index (r);
            write (Dir_Register32::DAT, v);
        }

    public:
        Ioapic (Paddr, unsigned, unsigned);

        ALWAYS_INLINE
        static inline bool claim_dev (unsigned r, unsigned i)
        {
            for (Ioapic *ioapic = list; ioapic; ioapic = ioapic->next)
                if (ioapic->rid == 0 && ioapic->id == i) {
                    ioapic->rid  = static_cast<uint16>(r);
                    return true;
                }

            return false;
        }

        ALWAYS_INLINE inline auto get_rid() const { return rid; }
        ALWAYS_INLINE inline auto get_gsi() const { return gsi_base; }

        ALWAYS_INLINE inline auto irt_max() { return read (Ind_Register32::VER) >> 16 & BIT_RANGE (7, 0); }
        ALWAYS_INLINE inline auto prq()     { return read (Ind_Register32::VER) >> 15 & BIT (0); }
        ALWAYS_INLINE inline auto version() { return read (Ind_Register32::VER)       & BIT_RANGE (7, 0); }

        ALWAYS_INLINE
        inline void set_rte_hi (unsigned gsi, uint32 v)
        {
            auto pin = gsi - gsi_base;
            write (Ind_Register32 (static_cast<unsigned>(Ind_Register32::RTE) + 2 * pin + 1), v);
        }

        ALWAYS_INLINE
        inline void set_rte_lo (unsigned gsi, uint32 v)
        {
            auto pin = gsi - gsi_base;
            write (Ind_Register32 (static_cast<unsigned>(Ind_Register32::RTE) + 2 * pin), v);
        }

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }
};
