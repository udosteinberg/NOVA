/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "memattr.hpp"
#include "mtd_arch.hpp"
#include "regs.hpp"

struct Sys_ipc_call final : private Sys_abi
{
    inline Sys_ipc_call (Sys_regs &r) : Sys_abi (r) {}

    inline bool timeout() const { return flags() & BIT (0); }

    inline unsigned long pt() const { return p0() >> 8; }

    inline Mtd_user mtd() const { return Mtd_user (uint32_t (p1())); }
};

struct Sys_ipc_reply final : private Sys_abi
{
    inline Sys_ipc_reply (Sys_regs &r) : Sys_abi (r) {}

    inline Mtd_arch mtd_a() const { return Mtd_arch (uint32_t (p1())); }

    inline Mtd_user mtd_u() const { return Mtd_user (uint32_t (p1())); }
};

struct Sys_create_pd final : private Sys_abi
{
    inline Sys_create_pd (Sys_regs &r) : Sys_abi (r) {}

    inline auto op() const { return flags(); }

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }
};

struct Sys_create_ec final : private Sys_abi
{
    inline Sys_create_ec (Sys_regs &r) : Sys_abi (r) {}

    inline auto flg() const { return flags(); }

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline cpu_t cpu() const { return p2() & OFFS_MASK (0); }

    inline uintptr_t hva() const { return p2() & ~OFFS_MASK (0); }

    inline uintptr_t sp() const { return p3(); }

    inline uintptr_t evt() const { return p4(); }
};

struct Sys_create_sc final : private Sys_abi
{
    inline Sys_create_sc (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline unsigned long ec() const { return p2(); }

    inline uint16_t budget() const { return p3() & BIT_RANGE (15, 0); }

    inline uint8_t prio() const { return p3() >> 16 & BIT_RANGE (6, 0); }

    inline cos_t cos() const { return p3() >> 23 & BIT_RANGE (15, 0); }
};

struct Sys_create_pt final : private Sys_abi
{
    inline Sys_create_pt (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline unsigned long ec() const { return p2(); }

    inline uintptr_t ip() const { return p3(); }
};

struct Sys_create_sm final : private Sys_abi
{
    inline Sys_create_sm (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sel() const { return p0() >> 8; }

    inline unsigned long pd() const { return p1(); }

    inline uint64_t cnt() const { return p2(); }
};

struct Sys_ctrl_pd final : private Sys_abi
{
    inline Sys_ctrl_pd (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long src() const { return p0() >> 8; }

    inline unsigned long dst() const { return p1(); }

    inline uintptr_t ssb() const { return p2() >> 12; }

    inline uintptr_t dsb() const { return p3() >> 12; }

    inline unsigned ord() const { return p2() & BIT_RANGE (4, 0); }

    inline unsigned pmm() const { return p3() & BIT_RANGE (4, 0); }

    inline auto ma() const { return Memattr { static_cast<uint32_t>(p4()) }; }
};

struct Sys_ctrl_ec final : private Sys_abi
{
    inline Sys_ctrl_ec (Sys_regs &r) : Sys_abi (r) {}

    inline bool strong() const { return flags() & BIT (0); }

    inline unsigned long ec() const { return p0() >> 8; }
};

struct Sys_ctrl_sc final : private Sys_abi
{
    inline Sys_ctrl_sc (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long sc() const { return p0() >> 8; }

    inline void set_time_ticks (uint64_t val) { p1() = val; }
};

struct Sys_ctrl_pt final : private Sys_abi
{
    inline Sys_ctrl_pt (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long pt() const { return p0() >> 8; }

    inline uintptr_t id() const { return p1(); }

    inline Mtd_arch mtd() const { return Mtd_arch (uint32_t (p2())); }
};

struct Sys_ctrl_sm final : private Sys_abi
{
    inline Sys_ctrl_sm (Sys_regs &r) : Sys_abi (r) {}

    inline bool op() const { return flags() & BIT (0); }

    inline bool zc() const { return flags() & BIT (1); }

    inline unsigned long sm() const { return p0() >> 8; }

    inline uint64_t time_ticks() const { return p1(); }
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

    inline auto cpu() const { return static_cast<uint16_t> (p1()); }

    inline auto dev() const { return static_cast<uint16_t> (p2()); }

    inline void set_msi_addr (uint32_t val) { p1() = val; }

    inline void set_msi_data (uint16_t val) { p2() = val; }
};

struct Sys_assign_dev final : private Sys_abi
{
    inline Sys_assign_dev (Sys_regs &r) : Sys_abi (r) {}

    inline unsigned long dma() const { return p0() >> 8; }

    inline uintptr_t smmu() const { return p1() & ~OFFS_MASK (0); }

    inline auto dad() const { return p2(); }
};
