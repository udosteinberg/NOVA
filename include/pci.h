/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
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

#include "compiler.h"
#include "io.h"
#include "slab.h"

class Dmar;

class Bdf
{
    private:
        uint32 const bdf;

        ALWAYS_INLINE
        inline void index (unsigned reg)
        {
            Io::out<uint32>(0xcf8, bdf | reg);
        }

    public:
        template <typename T>
        ALWAYS_INLINE
        inline unsigned read (unsigned reg)
        {
            index (reg);
            return Io::in<T>(0xcfc + (reg & 3));
        }

        template <typename T>
        ALWAYS_INLINE
        inline void write (unsigned reg, T val)
        {
            index (reg);
            Io::out<T>(0xcfc + (reg & 3), val);
        }

        ALWAYS_INLINE
        bool operator == (Bdf const &b) const { return bdf == b.bdf; }

        ALWAYS_INLINE
        inline Bdf (unsigned b, unsigned d, unsigned f) : bdf (1ul << 31 | b << 16 | d << 11 | f << 8) {}
};

class Pci
{
    friend class Acpi_table_mcfg;

    private:
        mword  const    reg_base;
        uint16 const    rid;
        uint16 const    level;
        Pci *           next;
        Dmar *          dmar;

        static Slab_cache   cache;
        static unsigned     bus_base;
        static Paddr        cfg_base;
        static size_t       cfg_size;
        static Pci *        list;

        template <typename T>
        ALWAYS_INLINE
        inline unsigned read (unsigned reg)
        {
            return *reinterpret_cast<T volatile *>(reg_base + reg);
        }

        template <typename T>
        ALWAYS_INLINE
        inline void write (unsigned reg, T val)
        {
            *reinterpret_cast<T volatile *>(reg_base + reg) = val;
        }

        ALWAYS_INLINE
        static inline Pci *find_dev (unsigned long r)
        {
            for (Pci *pci = list; pci; pci = pci->next)
                if (pci->rid == r)
                    return pci;

            return 0;
        }

    public:
        INIT
        Pci (unsigned, unsigned, unsigned, unsigned);

        ALWAYS_INLINE INIT
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE INIT
        static inline void claim_all (Dmar *d)
        {
            for (Pci *pci = list; pci; pci = pci->next)
                if (!pci->dmar)
                    pci->dmar = d;
        }

        ALWAYS_INLINE INIT
        static inline bool claim_dev (Dmar *d, unsigned r)
        {
            Pci *pci = find_dev (r);

            if (!pci)
                return false;

            unsigned l = pci->level;
            do pci->dmar = d; while ((pci = pci->next) && pci->level > l);

            return true;
        }

        INIT
        static unsigned init (unsigned, unsigned = 0);

        ALWAYS_INLINE
        static inline unsigned phys_to_rid (Paddr phys)
        {
            return phys - cfg_base < cfg_size ? (bus_base << 8) + (phys - cfg_base) / PAGE_SIZE : ~0UL;
        }

        ALWAYS_INLINE
        static inline Dmar *find_dmar (unsigned long r)
        {
            Pci *pci = find_dev (r);

            return pci ? pci->dmar : 0;
        }
};
