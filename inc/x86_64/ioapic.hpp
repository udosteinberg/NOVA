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
#include "memory.hpp"
#include "slab.hpp"
#include "vectors.hpp"

class Ioapic : public List<Ioapic>
{
    private:
        uintptr_t   const       reg_base    { 0 };
        unsigned    const       gsi_base    { 0 };
        unsigned    const       id          { 0 };
        uint16                  bdf         { 0 };
        Spinlock                lock;

        static          Slab_cache  cache;                      // IOAPIC Slab Cache
        static inline   Ioapic *    list    { nullptr };        // IOAPIC List
        static inline   uintptr_t   mmap    { MMAP_GLB_APIC };  // IOAPIC Memory Map Pointer

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

        inline auto read  (Dir_Register32 r) const    { return *reinterpret_cast<uint32 volatile *>(reg_base + static_cast<unsigned>(r)); }
        inline void write (Dir_Register32 r, uint32 v) const { *reinterpret_cast<uint32 volatile *>(reg_base + static_cast<unsigned>(r)) = v; }
        inline void write (Dir_Register8  r, uint8  v) const { *reinterpret_cast<uint8  volatile *>(reg_base + static_cast<unsigned>(r)) = v; }

        ALWAYS_INLINE
        inline void index (Ind_Register32 r) const
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

        void init();

    public:
        explicit Ioapic (Paddr, unsigned, unsigned);

        ALWAYS_INLINE
        static inline void init_all()
        {
            for (auto l = list; l; l = l->next)
                l->init();
        }

        ALWAYS_INLINE
        static inline bool claim_dev (unsigned r, unsigned i)
        {
            for (auto l = list; l; l = l->next)
                if (l->bdf == 0 && l->id == i) {
                    l->bdf = static_cast<uint16>(r);
                    return true;
                }

            return false;
        }

        ALWAYS_INLINE inline auto rid() const { return bdf; }
        ALWAYS_INLINE inline auto mre() { return read (Ind_Register32::VER) >> 16 & BIT_RANGE (7, 0); }
        ALWAYS_INLINE inline auto ver() { return read (Ind_Register32::VER)       & BIT_RANGE (7, 0); }

        ALWAYS_INLINE
        inline void set_dst (unsigned gsi, uint32 v)
        {
            auto rte = gsi - gsi_base;
            write (Ind_Register32 (static_cast<unsigned>(Ind_Register32::RTE) + 2 * rte + 1), v);
        }

        ALWAYS_INLINE
        inline void set_cfg (unsigned gsi, bool msk = true, bool trg = false, bool pol = false)
        {
            auto rte = gsi - gsi_base;
            write (Ind_Register32 (static_cast<unsigned>(Ind_Register32::RTE) + 2 * rte), msk << 16 | trg << 15 | pol << 13 | ((VEC_GSI + gsi) & BIT_RANGE (7, 0)));
        }

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }
};
