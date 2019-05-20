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

#include "mtd.hpp"

class Sys_ipc_call : public Sys_regs
{
    public:
        unsigned long pt() const { return r[0] >> 8; }

        bool timeout() const { return flags() & BIT (0); }

        Mtd_user mtd() const { return Mtd_user (r[1]); }
};

class Sys_ipc_reply : public Sys_regs
{
    public:
        Mtd_arch mtd_a() const { return Mtd_arch (r[1]); }
        Mtd_user mtd_u() const { return Mtd_user (r[1]); }
};

class Sys_create_pd : public Sys_regs
{
    public:
        unsigned long sel() const { return r[0] >> 8; }

        unsigned long own() const { return r[1]; }
};

class Sys_create_ec : public Sys_regs
{
    public:
        bool glb() const { return flags() & BIT (0); }

        bool vcpu() const { return flags() & BIT (1); }

        bool fpu() const { return flags() & BIT (2); }

        unsigned long sel() const { return r[0] >> 8; }

        unsigned long own() const { return r[1]; }

        unsigned cpu() const { return r[2] & 0xfff; }

        mword utcb() const { return r[2] & ~0xfff; }

        mword sp() const { return r[3]; }

        mword eb() const { return r[4]; }
};

class Sys_create_sc : public Sys_regs
{
    public:
        unsigned long sel() const { return r[0] >> 8; }

        unsigned long own() const { return r[1]; }

        unsigned long ec() const { return r[2]; }

        unsigned quantum() const { return unsigned (r[3] >> 12); }

        unsigned prio() const { return r[3] & BIT_RANGE (6, 0); }
};

class Sys_create_pt : public Sys_regs
{
    public:
        unsigned long sel() const { return r[0] >> 8; }

        unsigned long own() const { return r[1]; }

        unsigned long ec() const { return r[2]; }

        mword ip() const { return r[3]; }
};

class Sys_create_sm : public Sys_regs
{
    public:
        unsigned long sel() const { return r[0] >> 8; }

        unsigned long own() const { return r[1]; }

        mword cnt() const { return r[2]; }
};

class Sys_ctrl_pd : public Sys_regs
{
    public:
        unsigned long spd() const { return r[0] >> 8; }

        unsigned long dpd() const { return r[1]; }

        mword src() const { return r[2] >> 12; }

        mword dst() const { return r[3] >> 12; }

        Space::Type st() const { return Space::Type (r[2] & BIT_RANGE (1, 0)); }

        Space::Index si() const { return Space::Index (r[3] & BIT_RANGE (1, 0)); }

        unsigned ord() const { return r[2] >> 2 & BIT_RANGE (4, 0); }

        unsigned pmm() const { return r[3] >> 2 & BIT_RANGE (4, 0); }

        Memattr::Shareability sh() const { return Memattr::Shareability (r[3] >> 10 & BIT_RANGE (1, 0)); }

        Memattr::Cacheability ca() const { return Memattr::Cacheability (r[3] >> 7 & BIT_RANGE (2, 0)); }
};

class Sys_ctrl_ec : public Sys_regs
{
    public:
        unsigned long ec() const { return r[0] >> 8; }

        bool strong() const { return flags() & BIT (0); }
};

class Sys_ctrl_sc : public Sys_regs
{
    public:
        unsigned long sc() const { return r[0] >> 8; }

        void set_time_ticks (uint64 val) { r[1] = val; }
};

class Sys_ctrl_pt : public Sys_regs
{
    public:
        unsigned long pt() const { return r[0] >> 8; }

        mword id() const { return r[1]; }

        Mtd_arch mtd() const { return Mtd_arch (r[2]); }
};

class Sys_ctrl_sm : public Sys_regs
{
    public:
        unsigned long sm() const { return r[0] >> 8; }

        bool op() const { return flags() & BIT (0); }

        bool zc() const { return flags() & BIT (1); }

        uint64 time_ticks() const { return r[1]; }
};

class Sys_ctrl_hw : public Sys_regs
{
    public:
        unsigned op() const { return flags(); }

        uint32 arg0() { return uint32 (r[1]); }
        mword &arg1() { return r[2]; }
        mword &arg2() { return r[3]; }
        mword &arg3() { return r[4]; }
        mword  arg4() { return r[5]; }
        mword  arg5() { return r[6]; }
        mword  arg6() { return r[7]; }

        void set_res (mword val) { r[1] = val; }
};

class Sys_assign_int : public Sys_regs
{
    public:
        bool msk() const { return flags() & BIT (0); }

        bool trg() const { return flags() & BIT (1); }

        bool gst() const { return flags() & BIT (3); }

        unsigned long sm() const { return r[0] >> 8; }

        unsigned cpu() const { return unsigned (r[1]); }
};

class Sys_assign_dev : public Sys_regs
{
    public:
        unsigned long pd() const { return r[0] >> 8; }

        unsigned ctx() const { return unsigned (r[1]); }

        unsigned smg() const { return unsigned (r[2]); }

        uint16 sid() const { return uint16 (r[3] >> 16); }

        Space::Index si() const { return Space::Index (r[3] & BIT_RANGE (1, 0)); }
};
