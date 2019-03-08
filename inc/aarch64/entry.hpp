/*
 * Entry/Exit Functions
 *
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
 *
 */

#pragma once

#define DECR_STACK          sub      sp,  sp,      #16*19
#define INCR_STACK          add      sp,  sp,      #16*19

#define SAVE_STATE_IRQ      SAVE_STATE                       \
                            stp     xzr, xzr, [sp, #16*18]  ;

#define SAVE_STATE_EXC      SAVE_STATE                       \
                            mrs     x14,           esr_el2  ;\
                            mrs     x15,           far_el2  ;\
                            stp     x14, x15, [sp, #16*18]  ;

#define SAVE_STATE          stp      x0,  x1, [sp, #16* 0]  ;\
                            stp      x2,  x3, [sp, #16* 1]  ;\
                            stp      x4,  x5, [sp, #16* 2]  ;\
                            stp      x6,  x7, [sp, #16* 3]  ;\
                            stp      x8,  x9, [sp, #16* 4]  ;\
                            stp     x10, x11, [sp, #16* 5]  ;\
                            stp     x12, x13, [sp, #16* 6]  ;\
                            stp     x14, x15, [sp, #16* 7]  ;\
                            stp     x16, x17, [sp, #16* 8]  ;\
                            stp     x18, x19, [sp, #16* 9]  ;\
                            stp     x20, x21, [sp, #16*10]  ;\
                            stp     x22, x23, [sp, #16*11]  ;\
                            stp     x24, x25, [sp, #16*12]  ;\
                            stp     x26, x27, [sp, #16*13]  ;\
                            stp     x28, x29, [sp, #16*14]  ;\
                            mrs      x9,            sp_el0  ;\
                            mrs     x10,         tpidr_el0  ;\
                            mrs     x11,       tpidrro_el0  ;\
                            mrs     x12,           elr_el2  ;\
                            mrs     x13,          spsr_el2  ;\
                            stp     x30,  x9, [sp, #16*15]  ;\
                            stp     x10, x11, [sp, #16*16]  ;\
                            stp     x12, x13, [sp, #16*17]  ;

#define LOAD_STATE          ldp     x12, x13, [sp, #16*17]  ;\
                            ldp     x10, x11, [sp, #16*16]  ;\
                            ldp     x30,  x9, [sp, #16*15]  ;\
                            msr     spsr_el2, x13           ;\
                            msr      elr_el2, x12           ;\
                            msr  tpidrro_el0, x11           ;\
                            msr    tpidr_el0, x10           ;\
                            msr       sp_el0,  x9           ;\
                            ldp     x28, x29, [sp, #16*14]  ;\
                            ldp     x26, x27, [sp, #16*13]  ;\
                            ldp     x24, x25, [sp, #16*12]  ;\
                            ldp     x22, x23, [sp, #16*11]  ;\
                            ldp     x20, x21, [sp, #16*10]  ;\
                            ldp     x18, x19, [sp, #16* 9]  ;\
                            ldp     x16, x17, [sp, #16* 8]  ;\
                            ldp     x14, x15, [sp, #16* 7]  ;\
                            ldp     x12, x13, [sp, #16* 6]  ;\
                            ldp     x10, x11, [sp, #16* 5]  ;\
                            ldp      x8,  x9, [sp, #16* 4]  ;\
                            ldp      x6,  x7, [sp, #16* 3]  ;\
                            ldp      x4,  x5, [sp, #16* 2]  ;\
                            ldp      x2,  x3, [sp, #16* 1]  ;\
                            ldp      x0,  x1, [sp, #16* 0]  ;
