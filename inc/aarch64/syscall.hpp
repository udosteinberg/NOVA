/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "memattr.hpp"
#include "mtd_arch.hpp"
#include "regs.hpp"
#include "space.hpp"

class Sys_ipc_call final : public Sys_regs
{
    public:
        bool timeout() const { return flags() & BIT (0); }

        unsigned long pt() const { return p0() >> 8; }

        Mtd_user mtd() const { return Mtd_user (uint32 (p1())); }
};

class Sys_ipc_reply final : public Sys_regs
{
    public:
        Mtd_arch mtd_a() const { return Mtd_arch (uint32 (p1())); }
        Mtd_user mtd_u() const { return Mtd_user (uint32 (p1())); }
};

class Sys_create_pd final : public Sys_regs
{
    public:
        unsigned long sel() const { return p0() >> 8; }

        unsigned long own() const { return p1(); }
};

class Sys_create_ec final : public Sys_regs
{
    public:
        bool glb() const { return flags() & BIT (0); }

        bool vcpu() const { return flags() & BIT (1); }

        bool fpu() const { return flags() & BIT (2); }

        unsigned long sel() const { return p0() >> 8; }

        unsigned long own() const { return p1(); }

        unsigned cpu() const { return p2() & 0xfff; }

        mword utcb() const { return p2() & ~0xfff; }

        mword sp() const { return p3(); }

        mword eb() const { return p4(); }
};

class Sys_create_sc final : public Sys_regs
{
    public:
        unsigned long sel() const { return p0() >> 8; }

        unsigned long own() const { return p1(); }

        unsigned long ec() const { return p2(); }

        unsigned quantum() const { return unsigned (p3() >> 12); }

        unsigned prio() const { return p3() & BIT_RANGE (6, 0); }
};

class Sys_create_pt final : public Sys_regs
{
    public:
        unsigned long sel() const { return p0() >> 8; }

        unsigned long own() const { return p1(); }

        unsigned long ec() const { return p2(); }

        mword ip() const { return p3(); }
};

class Sys_create_sm final : public Sys_regs
{
    public:
        unsigned long sel() const { return p0() >> 8; }

        unsigned long own() const { return p1(); }

        mword cnt() const { return p2(); }
};

class Sys_ctrl_pd final : public Sys_regs
{
    public:
        unsigned long spd() const { return p0() >> 8; }

        unsigned long dpd() const { return p1(); }

        mword src() const { return p2() >> 12; }

        mword dst() const { return p3() >> 12; }

        Space::Type st() const { return Space::Type (p2() & BIT_RANGE (1, 0)); }

        Space::Index si() const { return Space::Index (p3() & BIT_RANGE (1, 0)); }

        unsigned ord() const { return p2() >> 2 & BIT_RANGE (4, 0); }

        unsigned pmm() const { return p3() >> 2 & BIT_RANGE (4, 0); }

        Memattr::Shareability sh() const { return Memattr::Shareability (p3() >> 10 & BIT_RANGE (1, 0)); }

        Memattr::Cacheability ca() const { return Memattr::Cacheability (p3() >> 7 & BIT_RANGE (2, 0)); }
};

class Sys_ctrl_ec final : public Sys_regs
{
    public:
        bool strong() const { return flags() & BIT (0); }

        unsigned long ec() const { return p0() >> 8; }
};

class Sys_ctrl_sc final : public Sys_regs
{
    public:
        unsigned long sc() const { return p0() >> 8; }

        void set_time_ticks (uint64 val) { p1() = val; }
};

class Sys_ctrl_pt final : public Sys_regs
{
    public:
        unsigned long pt() const { return p0() >> 8; }

        mword id() const { return p1(); }

        Mtd_arch mtd() const { return Mtd_arch (uint32 (p2())); }
};

class Sys_ctrl_sm final : public Sys_regs
{
    public:
        bool op() const { return flags() & BIT (0); }

        bool zc() const { return flags() & BIT (1); }

        unsigned long sm() const { return p0() >> 8; }

        uint64 time_ticks() const { return p1(); }
};

class Sys_ctrl_hw final : public Sys_regs
{
    public:
        unsigned op() const { return flags(); }
};

class Sys_assign_int final : public Sys_regs
{
    public:
        unsigned long sm() const { return p0() >> 8; }

        unsigned cpu() const { return unsigned (p1()); }

        uint16 dev() const { return uint16 (p2()); }

        void set_msi_addr (uint32 val) { p1() = val; }

        void set_msi_data (uint16 val) { p2() = val; }
};

class Sys_assign_dev final : public Sys_regs
{
    public:
        unsigned long pd() const { return p0() >> 8; }

        mword smmu() const { return p1() & ~0xfff; }

        Space::Index si() const { return Space::Index (p1() & BIT_RANGE (1, 0)); }

        mword dev() const { return p2(); }
};
