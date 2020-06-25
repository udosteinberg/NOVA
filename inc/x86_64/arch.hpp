/*
 * Architecture Definitions
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#define ARCH                "x86_64"
#define BFD_ARCH            "i386:x86-64"
#define BFD_FORMAT          "elf64-x86-64"
#define ELF_PHDR            Ph64
#define ELF_CLASS           2
#define ELF_MACHINE         62
#define PTE_BPL             9
#define ARG_IP              rcx
#define ARG_SP              r11
#define ARG_1               rdi
#define ARG_2               rsi
#define ARG_3               rdx
#define ARG_4               rax
#define ARG_5               r8
