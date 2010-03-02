/*
 * System-Call Interface
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
#include "crd.h"
#include "mtd.h"
#include "qpd.h"
#include "regs.h"
#include "types.h"

class Sys_ipc_send : public Exc_regs
{
    public:
        enum
        {
            DISABLE_BLOCKING    = 1ul << 0,
            DISABLE_DONATION    = 1ul << 1,
            DISABLE_REPLYCAP    = 1ul << 2
        };

        ALWAYS_INLINE
        inline unsigned flags() const { return eax >> 8 & 0xff; }

        ALWAYS_INLINE
        inline unsigned long pt() const { return edi; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (esi); }
};

class Sys_ipc_repl : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (esi); }
};

class Sys_create_pd : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned flags() const { return eax >> 8 & 0xff; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long cpu() const { return esi & 0xfff; }

        ALWAYS_INLINE
        inline mword utcb() const { return esi & ~0xfff; }

        ALWAYS_INLINE
        inline Qpd qpd() const { return Qpd (ebx); }

        ALWAYS_INLINE
        inline Crd crd() const { return Crd (ebp); }
};

class Sys_create_ec : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned flags() const { return eax >> 8 & 0xff; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long cpu() const { return esi & 0xfff; }

        ALWAYS_INLINE
        inline mword utcb() const { return esi & ~0xfff; }

        ALWAYS_INLINE
        inline mword esp() const { return ebx; }

        ALWAYS_INLINE
        inline mword evt() const { return ebp; }
};

class Sys_create_sc : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sc() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return esi; }

        ALWAYS_INLINE
        inline Qpd qpd() const { return Qpd (ebx); }
};

class Sys_create_pt : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long pt() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return esi; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (ebx); }

        ALWAYS_INLINE
        inline mword eip() const { return ebp; }
};

class Sys_create_sm : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return edi; }

        ALWAYS_INLINE
        inline mword cnt() const { return esi; }
};

class Sys_revoke : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned flags() const { return eax >> 8 & 0xff; }

        ALWAYS_INLINE
        inline Crd crd() const { return Crd (edi); }
};

class Sys_recall : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long ec() const { return edi; }
};

class Sys_semctl : public Exc_regs
{
    public:
        enum
        {
            UP,
            DN
        };

        ALWAYS_INLINE
        inline unsigned flags() const { return eax >> 8 & 0xff; }

        ALWAYS_INLINE
        inline unsigned long sm() const { return edi; }
};

class Sys_assign_pci : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long pd() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long pf() const { return esi; }

        ALWAYS_INLINE
        inline unsigned long vf() const { return ebx; }
};

class Sys_assign_gsi : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return edi; }

        ALWAYS_INLINE
        inline unsigned long cpu() const { return esi; }

        ALWAYS_INLINE
        inline unsigned long rid() const { return ebx; }

        ALWAYS_INLINE
        inline void set_msi (uint64 val)
        {
            edi = static_cast<mword>(val >> 32);
            esi = static_cast<mword>(val);
        }
};
