/*
 * Generic Macros
 *
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

#ifdef __ASSEMBLER__
#define SFX_U(X)            X
#define SFX_UL(X)           X
#define SFX_ULL(X)          X
#else
#define SFX_U(X)            X ## U
#define SFX_UL(X)           X ## UL
#define SFX_ULL(X)          X ## ULL
#endif

#define VAL_SHIFT(V,S)      (SFX_U   (V) << (S))
#define VALN_SHIFT(V,S)     (SFX_UL  (V) << (S))
#define VAL64_SHIFT(V,S)    (SFX_ULL (V) << (S))

#define BIT_RANGE(H,L)      (VAL_SHIFT   (2, H) - VAL_SHIFT   (1, L))
#define BITN_RANGE(H,L)     (VALN_SHIFT  (2, H) - VALN_SHIFT  (1, L))
#define BIT64_RANGE(H,L)    (VAL64_SHIFT (2, H) - VAL64_SHIFT (1, L))

#define BIT(B)              VAL_SHIFT   (1, B)
#define BITN(B)             VALN_SHIFT  (1, B)
#define BIT64(B)            VAL64_SHIFT (1, B)
