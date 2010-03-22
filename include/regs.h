/*
 * Register File
 *
 * Copyright (C) 2008-2010, Udo Steinberg <udo@hypervisor.org>
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

class Vmcb;
class Vmcs;
class Vtlb;

class Sys_regs
{
    public:
        union {
            struct {
                mword   edi;                // 0x0
                mword   esi;                // 0x4
                mword   ebp;                // 0x8
                mword   cr2;                // 0xc
                mword   ebx;                // 0x10
                mword   edx;                // 0x14
                mword   ecx;                // 0x18
                mword   eax;                // 0x1c
            };
            mword gpr[];
        };

        enum Status
        {
            SUCCESS,
            TIMEOUT,
            BAD_SYS,
            BAD_CAP,
            BAD_MEM,
            BAD_FTR,
            BAD_CPU,
            BAD_DEV,
        };

        enum
        {
            CALL,
            REPLY,
            CREATE_PD,
            CREATE_EC,
            CREATE_SC,
            CREATE_PT,
            CREATE_SM,
            REVOKE,
            RECALL,
            SEMCTL,
            ASSIGN_PCI,
            ASSIGN_GSI,
        };

        ALWAYS_INLINE
        inline void set_status (Status status) { eax = status; }

        ALWAYS_INLINE
        inline void set_pt (mword pt) { eax = pt; }

        ALWAYS_INLINE
        inline void set_ip (mword ip) { edx = ip; }
};

class Exc_regs : public Sys_regs
{
    friend class Utcb;

    private:
        union {
            struct {
                mword   gs;         // 0x20
                mword   fs;         // 0x24
                mword   es;         // 0x28
                mword   ds;         // 0x2c
                mword   err;        // 0x30
                mword   vec;        // 0x34
                mword   eip;        // 0x38
                mword   cs;         // 0x3c
                mword   efl;        // 0x40
                mword   esp;        // 0x44
                mword   ss;         // 0x48
            };
            struct {
                union {
                    Vmcs *  vmcs;   // 0x20
                    Vmcb *  vmcb;   // 0x20
                };
                Vtlb *  vtlb;       // 0x24
                mword   cr0_shadow; // 0x28
                mword   cr3_shadow; // 0x2c
                mword   cr4_shadow; // 0x30
                mword   dst_portal; // 0x34
                mword   ept_fault;  // 0x38
                uint8   ept_on;     // 0x3c
                uint8   fpu_on;     // 0x3d
            };
        };

        ALWAYS_INLINE
        inline mword cr0_set() const;

        ALWAYS_INLINE
        inline mword cr0_msk() const;

        ALWAYS_INLINE
        inline mword cr4_set() const;

        ALWAYS_INLINE
        inline mword cr4_msk() const;

        ALWAYS_INLINE
        inline mword get_cr0() const;

        ALWAYS_INLINE
        inline mword get_cr3() const;

        ALWAYS_INLINE
        inline mword get_cr4() const;

        ALWAYS_INLINE
        inline void set_cr0 (mword);

        ALWAYS_INLINE
        inline void set_cr3 (mword);

        ALWAYS_INLINE
        inline void set_cr4 (mword);

        ALWAYS_INLINE
        inline void set_exc_bitmap() const;

    public:
        ALWAYS_INLINE
        inline bool user() const { return cs & 3; }

        void ept_ctrl (bool);
        void fpu_ctrl (bool);

        void set_cpu_ctrl0 (mword);
        void set_cpu_ctrl1 (mword);

        mword read_gpr (unsigned);
        void write_gpr (unsigned, mword);

        mword read_cr (unsigned);
        void write_cr (unsigned, mword);
};
