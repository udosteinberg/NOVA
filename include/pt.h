/*
 * Portal
 *
 * Copyright (C) 2007-2010, Udo Steinberg <udo@hypervisor.org>
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
#include "kobject.h"
#include "mtd.h"
#include "refptr.h"
#include "slab.h"
#include "types.h"
#include "vma.h"

class Ec;

class Pt : public Kobject
{
    private:
        static Slab_cache cache;

    public:
        Refptr<Ec>  ec;
        Mtd         mtd;
        mword       ip;
        Vma         vma;

        Pt (Ec *, Mtd, mword, Pd *, mword);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
