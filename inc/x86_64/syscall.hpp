/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "abi.hpp"
#include "mtd_arch.hpp"
#include "qpd.hpp"
#include "regs.hpp"

struct Sys_ipc_call final : private Sys_abi
{
    inline Sys_ipc_call (Sys_regs &r) : Sys_abi (r) {}

    inline bool timeout() const { return flags() & BIT (0); }

    inline unsigned long pt() const { return p0() >> 8; }

    inline Mtd_user mtd() const { return Mtd_user (uint32 (p1())); }
};

struct Sys_ipc_reply final : private Sys_abi
{
    inline Sys_ipc_reply (Sys_regs &r) : Sys_abi (r) {}

    inline Mtd_arch mtd_a() const { return Mtd_arch (uint32 (p1())); }

    inline Mtd_user mtd_u() const { return Mtd_user (uint32 (p1())); }
};

struct Sys_create_pd final : private Sys_abi
{
    inline Sys_create_pd (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }
};

struct Sys_create_ec final : private Sys_abi
{
    inline Sys_create_ec (Sys_regs &r) : Sys_abi (r) {}

    inline bool type() const { return flags() & BIT (0); }

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline unsigned cpu() const { return p2() & 0xfff; }

    inline mword utcb() const { return p2() & ~0xfff; }

    inline mword esp() const { return p3(); }

    inline unsigned evt() const { return static_cast<unsigned>(p4()); }
};

struct Sys_create_sc final : private Sys_abi
{
    inline Sys_create_sc (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline unsigned long ec() const { return p2(); }

    inline Qpd qpd() const { return Qpd (p3()); }
};

struct Sys_create_pt final : private Sys_abi
{
    inline Sys_create_pt (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline unsigned long ec() const { return p2(); }

    inline auto mtd() const { return Mtd_arch (static_cast<uint32> (p3())); }

    inline mword eip() const { return p4(); }
};

struct Sys_create_sm final : private Sys_abi
{
    inline Sys_create_sm (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline mword cnt() const { return p2(); }
};

struct Sys_ctrl_ec final : private Sys_abi
{
    inline Sys_ctrl_ec (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long ec() const { return p0() >> 8; }
};

struct Sys_ctrl_sc final : private Sys_abi
{
    inline Sys_ctrl_sc (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sc() const { return p0() >> 8; }

    inline void set_time (uint64 val)
    {
        p1() = static_cast<mword>(val >> 32);
        p2() = static_cast<mword>(val);
    }
};

struct Sys_ctrl_pt final : private Sys_abi
{
    inline Sys_ctrl_pt (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long pt() const { return p0() >> 8; }

    inline mword id() const { return p1(); }
};

struct Sys_ctrl_sm final : private Sys_abi
{
    inline Sys_ctrl_sm (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sm() const { return p0() >> 8; }

    inline unsigned op() const { return flags() & 0x1; }

    inline unsigned zc() const { return flags() & 0x2; }

    inline uint64 time() const { return static_cast<uint64>(p1()) << 32 | p2(); }
};

struct Sys_ctrl_hw final : private Sys_abi
{
    inline Sys_ctrl_hw (Sys_regs &r) : Sys_abi (r) {}

    inline auto op() const { return flags(); }

    inline auto desc() const { return p0() >> 8; }
};

struct Sys_assign_int final : private Sys_abi
{
    inline Sys_assign_int (Sys_regs &r) : Sys_abi (r) {}

    inline auto flg() const { return flags(); }

    inline unsigned long sm() const { return p0() >> 8; }

    inline auto dev() const { return p1(); }

    inline unsigned cpu() const { return static_cast<unsigned>(p2()); }

    inline void set_msi (uint64 val)
    {
        p1() = static_cast<mword>(val >> 32);
        p2() = static_cast<mword>(val);
    }
};

struct Sys_assign_dev final : private Sys_abi
{
    inline Sys_assign_dev (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long pd() const { return p0() >> 8; }

    inline uintptr_t smmu() const { return p1() & ~OFFS_MASK; }

    inline auto dad() const { return p2(); }
};
