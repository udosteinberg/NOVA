/*
 * Safer Mode Extensions (SMX)
 *
 * Copyright (C) 2007-2008, Udo Steinberg <udo@hypervisor.org>
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
#include "memory.h"
#include "mtrr.h"
#include "multiboot.h"
#include "types.h"

class Smx
{
    private:
        class AC_Module
        {
            public:
                uint32  type;
                uint32  len;
                uint32  version;
                uint32  id;
                uint32  vendor;
                uint32  date;
                uint32  size;
                uint32  reserved1;
                uint32  code_control;
                uint32  error_entry;
                uint32  gdt_limit;
                uint32  gdt_base;
                uint32  seg_sel;
                uint32  entry;
                uint32  reserved2[16];
                uint32  key_size;
                uint32  scratch_size;
        };

        class MDR
        {
            public:
                uint64  phys;
                uint64  len;
                uint8   type;
                uint8   reserved[7];
        };

        class BIOS_OS
        {
            public:
                uint64  size;
                uint32  version;
                uint32  sinit_size;
                uint64  lcp_pd_base;
                uint64  lcp_pd_size;
                uint32  num_cpu;
                uint32  flags[2];
        };

        class OS_MLE
        {
            public:
                uint64  size;
                mword   mbi;
                Mtrr    mtrr;
        };

        class OS_SINIT
        {
            public:
                uint64  size;
                uint32  version;
                uint32  reserved;
                uint64  mle_ptab;
                uint64  mle_size;
                uint64  mle_header;
                uint64  pmr_lo_base;
                uint64  pmr_lo_size;
                uint64  pmr_hi_base;
                uint64  pmr_hi_size;
                uint64  lcp_po_base;
                uint64  lcp_po_size;
                uint32  capabilities;
        };

        class SINIT_MLE
        {
            public:
                uint64  size;
                uint32  version;
                union {
                    struct {
                        uint32  mdr_count;
                        MDR     mdr[];
                    } v1;
                    struct {
                        char    bios_acm_id[20];
                        uint32  senter_flags;
                        uint32  mseg_valid[2];
                        uint8   hash_sinit[20];
                        uint8   hash_mle[20];
                        uint8   hash_stm[20];
                        uint8   hash_lcp[20];
                        uint32  policy_control;
                        uint32  rlp_wakeup;
                        uint32  reserved;
                        uint32  mdr_count;
                        uint32  mdr_offset;
                        uint32  dmar_size;
                        uint32  dmar_addr;
                    } v2;
                };
        };

        enum Space
        {
            TXT_PRIVATE             = 0xfed20000,
            TXT_PUBLIC              = 0xfed30000
        };

        enum Register
        {
            TXT_STS                 = 0x0,
            TXT_ESTS                = 0x8,
            TXT_THREADS_EXIST       = 0x10,
            TXT_THREADS_JOIN        = 0x20,
            TXT_ERRORCODE           = 0x30,
            TXT_CMD_RESET           = 0x38,
            TXT_CMD_OPN_PRIVATE     = 0x40,
            TXT_CMD_CLS_PRIVATE     = 0x48,
            TXT_VER_FSBIF           = 0x100,
            TXT_DIDVID              = 0x110,
            TXT_EID                 = 0x118,
            TXT_CMD_UNLOCK_MEMCFG   = 0x218,
            TXT_CMD_FLUSH_WB        = 0x258,
            TXT_NODMA_BASE          = 0x260,
            TXT_NODMA_SIZE          = 0x268,
            TXT_SINIT_BASE          = 0x270,
            TXT_SINIT_SIZE          = 0x278,
            TXT_MLE_JOIN            = 0x290,
            TXT_HEAP_BASE           = 0x300,
            TXT_HEAP_SIZE           = 0x308,
            TXT_CMD_OPN_LOCALITY1   = 0x380,
            TXT_CMD_CLS_LOCALITY1   = 0x388,
            TXT_CMD_OPN_LOCALITY2   = 0x390,
            TXT_CMD_CLS_LOCALITY2   = 0x398,
            TXT_CMD_SECRETS         = 0x8e0,
            TXT_CMD_NO_SECRETS      = 0x8e8,
            TXT_E2STS               = 0x8f0
        };

        enum Leaf
        {
            LEAF_CAPABILITIES       = 0,
            LEAF_ENTERACCS          = 2,
            LEAF_EXITAC             = 3,
            LEAF_SENTER             = 4,
            LEAF_SEXIT              = 5,
            LEAF_PARAMETERS         = 6,
            LEAF_SMCTRL             = 7,
            LEAF_WAKEUP             = 8
        };

        enum Capability
        {
            CAP_CHIPSET             = 1ul << LEAF_CAPABILITIES,
            CAP_ENTERACCS           = 1ul << LEAF_ENTERACCS,
            CAP_EXITAC              = 1ul << LEAF_EXITAC,
            CAP_SENTER              = 1ul << LEAF_SENTER,
            CAP_SEXIT               = 1ul << LEAF_SEXIT,
            CAP_PARAMETERS          = 1ul << LEAF_PARAMETERS,
            CAP_SMCTRL              = 1ul << LEAF_SMCTRL,
            CAP_WAKEUP              = 1ul << LEAF_WAKEUP
        };

        static unsigned const cap_req = CAP_CHIPSET |
                                        CAP_SENTER  |
                                        CAP_SMCTRL  |
                                        CAP_WAKEUP;

        template <typename T>
        ALWAYS_INLINE
        static inline T phys_read (Register reg)
        {
            return *reinterpret_cast<T volatile *>(TXT_PUBLIC + reg);
        }

        template <typename T>
        ALWAYS_INLINE
        static inline void phys_write (Register reg, T val)
        {
            *reinterpret_cast<T volatile *>(TXT_PUBLIC + reg) = val;
        }

        ALWAYS_INLINE
        static inline void capabilities (unsigned index, uint32 &eax)
        {
            asm volatile ("getsec"
                          : "=a" (eax)
                          : "a" (LEAF_CAPABILITIES), "b" (index));
        }

        ALWAYS_INLINE
        static inline void senter (mword base, mword size)
        {
            asm volatile ("getsec"
                          :
                          : "a" (LEAF_SENTER), "b" (base), "c" (size), "d" (0)
                          : "memory");
        }

        ALWAYS_INLINE
        static inline void parameters (unsigned index, uint32 &eax, uint32 &ebx, uint32 &ecx)
        {
            asm volatile ("getsec"
                          : "=a" (eax), "=b" (ebx), "=c" (ecx)
                          : "a" (LEAF_PARAMETERS), "b" (index));
        }

        ALWAYS_INLINE
        static inline void smctrl()
        {
            asm volatile ("getsec"
                          :
                          : "a" (LEAF_SMCTRL), "b" (0));
        }

        ALWAYS_INLINE
        static inline void wakeup()
        {
            asm volatile ("getsec"
                          :
                          : "a" (LEAF_WAKEUP));
        }

    public:
        INIT REGPARM (1)
        static void launch (Multiboot *mbi) asm ("secure_launch");
};
