/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "mtd.hpp"
#include "qpd.hpp"
#include "regs.hpp"

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
        inline unsigned long pt() const { return ARG_1 >> 8; }
};

class Sys_create_pd : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }
};

class Sys_create_ec : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned cpu() const { return ARG_3 & 0xfff; }

        ALWAYS_INLINE
        inline mword utcb() const { return ARG_3 & ~0xfff; }

        ALWAYS_INLINE
        inline mword esp() const { return ARG_4; }

        ALWAYS_INLINE
        inline unsigned evt() const { return static_cast<unsigned>(ARG_5); }
};

class Sys_create_sc : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return ARG_3; }

        ALWAYS_INLINE
        inline Qpd qpd() const { return Qpd (ARG_4); }
};

class Sys_create_pt : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return ARG_3; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (ARG_4); }

        ALWAYS_INLINE
        inline mword eip() const { return ARG_5; }
};

class Sys_create_sm : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline mword cnt() const { return ARG_3; }
};

class Sys_ec_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long ec() const { return ARG_1 >> 8; }
};

class Sys_sc_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sc() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline void set_time (uint64 val)
        {
            ARG_2 = static_cast<mword>(val >> 32);
            ARG_3 = static_cast<mword>(val);
        }
};

class Sys_pt_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long pt() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline mword id() const { return ARG_2; }
};

class Sys_sm_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned op() const { return flags() & 0x1; }

        ALWAYS_INLINE
        inline unsigned zc() const { return flags() & 0x2; }

        ALWAYS_INLINE
        inline uint64 time() const { return static_cast<uint64>(ARG_2) << 32 | ARG_3; }
};

class Sys_assign_gsi : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline mword dev() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned cpu() const { return static_cast<unsigned>(ARG_3); }

        ALWAYS_INLINE
        inline void set_msi (uint64 val)
        {
            ARG_2 = static_cast<mword>(val >> 32);
            ARG_3 = static_cast<mword>(val);
        }
};

class Sys_assign_dev final : public Sys_regs
{
    public:
        inline unsigned long pd() const { return ARG_1 >> 8; }

        inline uintptr_t smmu() const { return ARG_2 & ~OFFS_MASK; }

        inline Space::Index si() const { return Space::Index (ARG_2 & BIT_RANGE (1, 0)); }

        inline uintptr_t dev() const { return ARG_3; }
};
