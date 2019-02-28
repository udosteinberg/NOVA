/*
 * Compiler Macros
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#define STRING(...) #__VA_ARGS__
#define EXPAND(X) STRING(X)

#if defined(__GNUC__)

        #define COMPILER                "gcc " __VERSION__

    #if defined(__GNUC_PATCHLEVEL__)
        #define COMPILER_STRING         "gcc " EXPAND (__GNUC__) "." EXPAND (__GNUC_MINOR__) "." EXPAND (__GNUC_PATCHLEVEL__)
        #define COMPILER_VERSION        (__GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__)
    #else
        #define COMPILER_STRING         "gcc " EXPAND (__GNUC__) "." EXPAND (__GNUC_MINOR__)
        #define COMPILER_VERSION        (__GNUC__ * 100 + __GNUC_MINOR__ * 10)
    #endif

    #ifndef __has_cpp_attribute
        #define __has_cpp_attribute(X)  0
    #endif

        #define COLD                    __attribute__((cold))
        #define HOT                     __attribute__((hot))

        #define ALIGNED(X)              __attribute__((aligned(X)))
        #define ALWAYS_INLINE           __attribute__((always_inline))
    #if defined(__x86_64__)
        #define CPULOCAL                __attribute__((section (".cpulocal,\"w\",@nobits#")))
        #define CPULOCAL_HOT            __attribute__((section (".cpulocal.hot,\"w\",@nobits#")))
        #define EFICALL                 __attribute__((ms_abi))
    #elif defined (__aarch64__)
        #define CPULOCAL                __attribute__((section (".cpulocal,\"w\",@nobits//")))
        #define CPULOCAL_HOT            __attribute__((section (".cpulocal.hot,\"w\",@nobits//")))
        #define EFICALL
    #endif
        #define FORMAT(X,Y)             __attribute__((format (printf, (X),(Y))))
        #define INIT                    __attribute__((section (".init")))
        #define INIT_DATA               __attribute__((section (".init.data")))
        #define INIT_PRIORITY(X)        __attribute__((init_priority((X))))
        #define NOINLINE                __attribute__((noinline))
        #define NONNULL                 __attribute__((nonnull))

        #define EXPECT_FALSE(X)         __builtin_expect(!!(X), 0)
        #define EXPECT_TRUE(X)          __builtin_expect(!!(X), 1)
        #define UNREACHED               __builtin_unreachable()

        #define ACCESS_ONCE(x)          (*static_cast<volatile decltype(x) *>(&(x)))

#else
        #define COMPILER                "unknown compiler"
        #define COMPILER_VERSION        0
#endif
