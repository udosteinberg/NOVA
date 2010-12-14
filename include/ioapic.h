/*
 * I/O Advanced Programmable Interrupt Controller (I/O APIC)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "apic.h"
#include "compiler.h"
#include "lock_guard.h"
#include "slab.h"

class Dmar;

class Ioapic : public Apic
{
    private:
        mword    const  reg_base;
        unsigned const  gsi_base;
        unsigned const  id;
        Ioapic *        next;
        Dmar *          dmar;
        uint16          rid;
        Spinlock        lock;

        static Slab_cache cache;
        static Ioapic *list;

        enum
        {
            IOAPIC_IDX  = 0x0,
            IOAPIC_WND  = 0x10,
            IOAPIC_PAR  = 0x20,
            IOAPIC_EOI  = 0x40,
        };

        enum Register
        {
            IOAPIC_ID   = 0x0,
            IOAPIC_VER  = 0x1,
            IOAPIC_ARB  = 0x2,
            IOAPIC_BCFG = 0x3,
            IOAPIC_IRT  = 0x10,
        };

        ALWAYS_INLINE
        inline void index (Register reg)
        {
            *reinterpret_cast<uint8 volatile *>(reg_base + IOAPIC_IDX) = reg;
        }

        ALWAYS_INLINE
        inline uint32 read (Register reg)
        {
            Lock_guard <Spinlock> guard (lock);
            index (reg);
            return *reinterpret_cast<uint32 volatile *>(reg_base + IOAPIC_WND);
        }

        ALWAYS_INLINE
        inline void write (Register reg, uint32 val)
        {
            Lock_guard <Spinlock> guard (lock);
            index (reg);
            *reinterpret_cast<uint32 volatile *>(reg_base + IOAPIC_WND) = val;
        }

    public:
        INIT
        Ioapic (Paddr, unsigned, unsigned);

        ALWAYS_INLINE INIT
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE INIT
        static inline void operator delete (void *ptr) { cache.free (ptr); }

        ALWAYS_INLINE INIT
        static inline bool claim_dev (Dmar *d, unsigned r, unsigned i)
        {
            for (Ioapic *ioapic = list; ioapic; ioapic = ioapic->next)
                if (ioapic->id == i) {
                    ioapic->rid  = static_cast<uint16>(r);
                    ioapic->dmar = d;
                    return true;
                }

            return false;
        }

        ALWAYS_INLINE
        inline uint16 get_rid() const { return rid; }

        ALWAYS_INLINE
        inline unsigned get_gsi() const { return gsi_base; }

        ALWAYS_INLINE
        inline unsigned version() { return read (IOAPIC_VER) & 0xff; }

        ALWAYS_INLINE
        inline unsigned prq() { return read (IOAPIC_VER) >> 15 & 0x1; }

        ALWAYS_INLINE
        inline unsigned irt_max() { return read (IOAPIC_VER) >> 16 & 0xff; }

        ALWAYS_INLINE
        inline void set_irt (unsigned gsi, unsigned val)
        {
            unsigned pin = gsi - gsi_base;
            write (Register (IOAPIC_IRT + 2 * pin), val);
        }

        ALWAYS_INLINE
        inline void set_cpu (unsigned gsi, unsigned cpu)
        {
            unsigned pin = gsi - gsi_base;
            write (Register (IOAPIC_IRT + 2 * pin + 1), cpu << 24 | gsi << 17 | 1ul << 16);
        }
};
