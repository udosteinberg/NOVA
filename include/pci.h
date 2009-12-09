/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009, Udo Steinberg <udo@hypervisor.org>
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

#include "assert.h"
#include "compiler.h"
#include "dmar.h"
#include "io.h"
#include "slab.h"

class Pd;

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
        unsigned bus() const { return bdf >> 16 & 0xff; }

        ALWAYS_INLINE
        unsigned dev() const { return bdf >> 11 & 0x1f; }

        ALWAYS_INLINE
        unsigned fun() const { return bdf >> 8 & 0x7; }

        ALWAYS_INLINE
        inline Bdf (unsigned b, unsigned d, unsigned f) : bdf (1ul << 31 | b << 16 | d << 11 | f << 8) {}
};

class Pci : public Bdf
{
    private:
        Pci *       next;
        unsigned    level;
        Dmar *      dmar;
        Pd *        pd;

        static Slab_cache cache;
        static Pci *list;

    public:
        Pci (unsigned, unsigned, unsigned, unsigned);

        ALWAYS_INLINE
        static inline void claim_all (Dmar *d)
        {
            for (Pci *pci = list; pci; pci = pci->next)
                if (pci->dmar == 0)
                    pci->dmar = d;
        }

        ALWAYS_INLINE
        static inline bool claim_dev (Dmar *d, Bdf bdf)
        {
            for (Pci *pci = list; pci; pci = pci->next)
                if (pci->dmar == 0 && *pci == bdf) {
                    pci->dmar = d;
                    return true;
                }

            return false;
        }

        ALWAYS_INLINE
        static inline bool assign_dev (Pd *p, Bdf bdf)
        {
            for (Pci *pci = list; pci; pci = pci->next)
                if (pci->pd == 0 && *pci == bdf && pci->dmar) {
                    pci->pd = p;
                    pci->dmar->assign (bdf.bus(), bdf.dev(), bdf.fun(), p);
                    return true;
                }

            return false;
        }

        INIT
        static void init (unsigned = 0, unsigned = 0);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
