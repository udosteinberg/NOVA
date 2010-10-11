/*
 * Register File
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

#include "atomic.h"
#include "compiler.h"
#include "hazards.h"
#include "types.h"

class Vmcb;
class Vmcs;
class Vtlb;

class Sys_regs
{
    protected:
        union {
            struct {
                mword   edi;
                mword   esi;
                mword   ebp;
                mword   cr2;
                mword   ebx;
                mword   edx;
                mword   ecx;
                mword   eax;
            };
            mword gpr[];
        };

    public:
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
            LOOKUP,
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
    protected:
        union {
            struct {
                mword   gs;
                mword   fs;
                mword   es;
                mword   ds;
                mword   err;
                mword   vec;
                mword   eip;
                mword   cs;
                mword   efl;
                mword   esp;
                mword   ss;
            };
            struct {
                union {
                    Vmcs *  vmcs;
                    Vmcb *  vmcb;
                };
                Vtlb *  vtlb;
                mword   cr0_shadow;
                mword   cr3_shadow;
                mword   cr4_shadow;
                mword   dst_portal;
                mword   ept_fault;
                mword   ept_error;
                uint8   ept_on;
                uint8   fpu_on;
            };
        };

    private:
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

        void load_pdpte();

        mword read_gpr (unsigned);
        void write_gpr (unsigned, mword);

        mword read_cr (unsigned);
        void write_cr (unsigned, mword);
};

class Cpu_regs : public Exc_regs
{
    private:
        mword   hzd;

    public:
        uint64  tsc_offset;
        mword   mtd;

        ALWAYS_INLINE
        inline mword hazard() const { return hzd; }

        ALWAYS_INLINE
        inline void set_hazard (mword h) { Atomic::set_mask<true>(hzd, h); }

        ALWAYS_INLINE
        inline void clr_hazard (mword h) { Atomic::clr_mask<true>(hzd, h); }

        ALWAYS_INLINE
        inline void add_tsc_offset (uint64 tsc)
        {
            tsc_offset += tsc;
            set_hazard (HZD_TSC);
        }
};
