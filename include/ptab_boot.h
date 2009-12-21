/*
 * IA32 Boot Page Table
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "paging.h"
#include "ptab.h"
#include "types.h"

class Ptab_boot : public Paging
{
    public:
        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            extern char _tables_p;
            static mword base INITDATA = reinterpret_cast<mword>(&_tables_p);
            base += 4096;
            return reinterpret_cast<void *>(base);
        }

        INIT
        static void init();

        INIT NOINLINE
        void map (Paddr phys, mword virt, size_t size, Attribute attrib);
};
