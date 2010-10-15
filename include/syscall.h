/*
 * System-Call Interface
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

#include "compiler.h"
#include "qpd.h"

class Sys_call : public Sys_regs
{
    public:
        enum
        {
            DISABLE_BLOCKING    = 1ul << 0,
            DISABLE_DONATION    = 1ul << 1,
            DISABLE_REPLYCAP    = 1ul << 2
        };

        ALWAYS_INLINE
        inline unsigned long pt() const { return eax >> 8; }
};

class Sys_create_pd : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return edi; }

        ALWAYS_INLINE
        inline Crd crd() const { return Crd (esi); }
};

class Sys_create_ec : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return edi; }

        ALWAYS_INLINE
        inline unsigned cpu() const { return esi & 0xfff; }

        ALWAYS_INLINE
        inline mword utcb() const { return esi & ~0xfff; }

        ALWAYS_INLINE
        inline mword esp() const { return ebx; }

        ALWAYS_INLINE
        inline unsigned evt() const { return static_cast<unsigned>(ebp); }
};

class Sys_create_sc : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return esi; }

        ALWAYS_INLINE
        inline Qpd qpd() const { return Qpd (ebx); }
};

class Sys_create_pt : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return esi; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (ebx); }

        ALWAYS_INLINE
        inline mword eip() const { return ebp; }
};

class Sys_create_sm : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return edi; }

        ALWAYS_INLINE
        inline mword cnt() const { return esi; }
};

class Sys_revoke : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline Crd crd() const { return Crd (edi); }
};

class Sys_lookup : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline Crd & crd() { return reinterpret_cast<Crd &>(edi); }
};

class Sys_recall : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long ec() const { return eax >> 8; }
};

class Sys_semctl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned op() const { return flags() & 0x1; }

        ALWAYS_INLINE
        inline unsigned zc() const { return flags() & 0x2; }
};

class Sys_assign_pci : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long pd() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned long pf() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long vf() const { return esi; }
};

class Sys_assign_gsi : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return eax >> 8; }

        ALWAYS_INLINE
        inline unsigned cpu() const { return static_cast<unsigned>(edi); }

        ALWAYS_INLINE
        inline unsigned rid() const { return static_cast<unsigned>(esi); }

        ALWAYS_INLINE
        inline void set_msi (uint64 val)
        {
            edi = static_cast<mword>(val >> 32);
            esi = static_cast<mword>(val);
        }
};
