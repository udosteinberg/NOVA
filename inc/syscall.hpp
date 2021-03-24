/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
        inline bool timeout() const { return flags() & BIT (0); }

        inline unsigned long pt() const { return p0() >> 8; }

        inline Mtd_user mtd() const { return Mtd_user (uint32 (p1())); }
};

class Sys_ipc_reply final : public Sys_regs
{
    public:
        inline Mtd_arch mtd_a() const { return Mtd_arch (uint32 (p1())); }

        inline Mtd_user mtd_u() const { return Mtd_user (uint32 (p1())); }
};

class Sys_create_pd final : public Sys_regs
{
    public:
        inline unsigned long sel() const { return p0() >> 8; }

        inline unsigned long own() const { return p1(); }
};

class Sys_create_ec final : public Sys_regs
{
    public:
        inline bool type() const { return flags() & BIT (0); }

        inline bool vcpu() const { return flags() & BIT (1); }

        inline bool fpu() const { return flags() & BIT (2); }

        inline unsigned long sel() const { return p0() >> 8; }

        inline unsigned long own() const { return p1(); }

        inline unsigned cpu() const { return p2() & OFFS_MASK; }

        inline uintptr_t utcb() const { return p2() & ~OFFS_MASK; }

        inline uintptr_t sp() const { return p3(); }

        inline uintptr_t eb() const { return p4(); }
};

class Sys_create_sc final : public Sys_regs
{
    public:
        inline unsigned long sel() const { return p0() >> 8; }

        inline unsigned long own() const { return p1(); }

        inline unsigned long ec() const { return p2(); }

        inline uint32 budget() const { return static_cast<uint32> (p3()) >> 12; }

        inline unsigned prio() const { return p3() & BIT_RANGE (6, 0); }
};

class Sys_create_pt final : public Sys_regs
{
    public:
        inline unsigned long sel() const { return p0() >> 8; }

        inline unsigned long own() const { return p1(); }

        inline unsigned long ec() const { return p2(); }

        inline uintptr_t ip() const { return p3(); }
};

class Sys_create_sm final : public Sys_regs
{
    public:
        inline unsigned long sel() const { return p0() >> 8; }

        inline unsigned long own() const { return p1(); }

        inline uint64 cnt() const { return p2(); }
};

class Sys_ctrl_pd final : public Sys_regs
{
    public:
        inline unsigned long spd() const { return p0() >> 8; }

        inline unsigned long dpd() const { return p1(); }

        inline uintptr_t src() const { return p2() >> 12; }

        inline uintptr_t dst() const { return p3() >> 12; }

        inline Space::Type st() const { return Space::Type (p2() & BIT_RANGE (1, 0)); }

        inline Space::Index si() const { return Space::Index (p3() & BIT_RANGE (1, 0)); }

        inline unsigned ord() const { return p2() >> 2 & BIT_RANGE (4, 0); }

        inline unsigned pmm() const { return p3() >> 2 & BIT_RANGE (4, 0); }

        inline Memattr::Shareability sh() const { return Memattr::Shareability (p3() >> 10 & BIT_RANGE (1, 0)); }

        inline Memattr::Cacheability ca() const { return Memattr::Cacheability (p3() >> 7 & BIT_RANGE (2, 0)); }
};

class Sys_ctrl_ec final : public Sys_regs
{
    public:
        inline bool strong() const { return flags() & BIT (0); }

        inline unsigned long ec() const { return p0() >> 8; }
};

class Sys_ctrl_sc final : public Sys_regs
{
    public:
        inline unsigned long sc() const { return p0() >> 8; }

        inline void set_time_ticks (uint64 val) { p1() = val; }
};

class Sys_ctrl_pt final : public Sys_regs
{
    public:
        inline unsigned long pt() const { return p0() >> 8; }

        inline uintptr_t id() const { return p1(); }

        inline Mtd_arch mtd() const { return Mtd_arch (uint32 (p2())); }
};

class Sys_ctrl_sm final : public Sys_regs
{
    public:
        inline bool op() const { return flags() & BIT (0); }

        inline bool zc() const { return flags() & BIT (1); }

        inline unsigned long sm() const { return p0() >> 8; }

        inline uint64 time_ticks() const { return p1(); }
};

class Sys_ctrl_pm final : public Sys_regs
{
    public:
        inline auto op() const { return flags(); }

        inline auto trans() const { return static_cast<uint16>(p0() >> 8); }
};

class Sys_assign_int final : public Sys_regs
{
    public:
        inline unsigned long sm() const { return p0() >> 8; }

        inline auto cpu() const { return static_cast<uint16> (p1()); }

        inline auto dev() const { return static_cast<uint16> (p2()); }

        inline void set_msi_addr (uint32 val) { p1() = val; }

        inline void set_msi_data (uint16 val) { p2() = val; }
};

class Sys_assign_dev final : public Sys_regs
{
    public:
        inline unsigned long pd() const { return p0() >> 8; }

        inline uintptr_t smmu() const { return p1() & ~OFFS_MASK; }

        inline Space::Index si() const { return Space::Index (p1() & BIT_RANGE (1, 0)); }

        inline uintptr_t dev() const { return p2(); }
};
