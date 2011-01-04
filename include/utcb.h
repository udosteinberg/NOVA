/*
 * User Thread Control Block (UTCB)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
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

#include "buddy.h"
#include "compiler.h"
#include "crd.h"

class Cpu_regs;

class Utcb_segment
{
    public:
        uint16  sel;
        uint16  ar;
        uint32  limit;
        union {
            uint64  : 64;
            mword   base;
        };

        ALWAYS_INLINE
        inline void set_vmx (mword s, mword b, mword l, mword a)
        {
            sel   = static_cast<uint16>(s);
            ar    = static_cast<uint16>((a >> 4 & 0x1f00) | (a & 0xff));
            limit = static_cast<uint32>(l);
            base  = static_cast<uint64>(b);
        }
};

class Utcb_head
{
    public:
        union {
            struct {
                uint16  ui, ti;
            };
            mword items;
        };
        Crd     crd;
        mword   tls;
        mword   res;
};

class Utcb_data
{
    protected:
        union {
            struct {
                mword           mtd, inst_len, rip, rflags;
                uint32          intr_state, actv_state;
                union {
                    struct {
                        uint32  intr_info, intr_error;
                    };
                    uint64      inj;
                };

                mword           rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
                uint64          qual[2];
                uint32          ctrl[2];
                uint64          tsc;
                mword           cr0, cr2, cr3, cr4;
                mword           dr7, sysenter_cs, sysenter_rsp, sysenter_rip;
                Utcb_segment    es, cs, ss, ds, fs, gs, ld, tr, gd, id;
            };

            mword mr[];
        };
};

class Utcb : public Utcb_head, private Utcb_data
{
    public:
        void load_exc (Cpu_regs *);
        void load_vmx (Cpu_regs *);
        void load_svm (Cpu_regs *);
        void save_exc (Cpu_regs *);
        void save_vmx (Cpu_regs *);
        void save_svm (Cpu_regs *);

        ALWAYS_INLINE NONNULL
        inline void save (Utcb *dst)
        {
            dst->items = items;
#if 0
            mword *d = dst->mr, *s = mr, u = ui;
            asm volatile ("rep; movsl" : "+D" (d), "+S" (s), "+c" (u) : : "memory");
#else
            for (unsigned long i = 0; i < ui; i++)
                dst->mr[i] = mr[i];
#endif
        }

        ALWAYS_INLINE
        inline Xfer *xfer() { return reinterpret_cast<Xfer *>(this) + PAGE_SIZE / sizeof (Xfer) - 1; }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }
};
