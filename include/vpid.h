/*
 * Virtual Processor Identifier (VPID)
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

#include "compiler.h"
#include "types.h"

class Invvpid
{
    public:
        uint64  vpid;
        uint64  addr;

        ALWAYS_INLINE
        inline Invvpid (unsigned long v, mword a) : vpid (v), addr (a) {}
};

class Vpid
{
    public:
        enum Type
        {
            ADDRESS             = 0,
            CONTEXT_GLOBAL      = 1,
            CONTEXT_NOGLOBAL    = 3
        };

        ALWAYS_INLINE
        static inline void flush (Type t, unsigned long vpid, mword addr = 0)
        {
            asm volatile ("invvpid %0, %1" : : "m" (Invvpid (vpid, addr)), "r" (static_cast<mword>(t)) : "cc");
        }
};
