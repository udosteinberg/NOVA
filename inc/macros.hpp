/*
 * Generic Macros
 *
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

#ifdef __ASSEMBLER__
#define SFX_U(X)            X
#define SFX_UL(X)           X
#define SFX_ULL(X)          X
#else
#define SFX_U(X)            X ## U
#define SFX_UL(X)           X ## UL
#define SFX_ULL(X)          X ## ULL
#endif

#define BIT(B)               (SFX_U   (1) << (B))
#define BIT64(B)             (SFX_ULL (1) << (B))
#define BIT_RANGE(H,L)      ((SFX_U   (2) << (H)) - (SFX_U   (1) << (L)))
#define BIT64_RANGE(H,L)    ((SFX_ULL (2) << (H)) - (SFX_ULL (1) << (L)))
