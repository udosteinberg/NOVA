/*
 * Compiler Macros
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

#define STRING(x) #x
#define EXPAND(x) STRING(x)

#if defined(__GNUC__)

        #define COMPILER            "gcc "__VERSION__
    #if defined(__GNUC_PATCHLEVEL__)
        #define COMPILER_STRING     "gcc " EXPAND(__GNUC__) "." EXPAND(__GNUC_MINOR__) "." EXPAND(__GNUC_PATCHLEVEL__)
        #define COMPILER_VERSION    (__GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__)
    #else
        #define COMPILER_STRING     "gcc " EXPAND(__GNUC__) "." EXPAND(__GNUC_MINOR__)
        #define COMPILER_VERSION    (__GNUC__ * 100 + __GNUC_MINOR__ * 10)
    #endif

    #if (COMPILER_VERSION < 430)
        #define COLD
        #define HOT
    #else
        #define COLD                __attribute__((cold))
        #define HOT                 __attribute__((hot))
    #endif

        #define ALIGNED(X)          __attribute__((aligned(X)))
        #define ALWAYS_INLINE       __attribute__((always_inline))
        #define CPULOCAL            __attribute__((section (".cpulocal,\"w\",@nobits#")))
        #define CPULOCAL_HOT        __attribute__((section (".cpulocal.hot,\"w\",@nobits#")))
        #define FORMAT(X,Y)         __attribute__((format (printf, (X),(Y))))
        #define INIT                __attribute__((section (".init")))
        #define INITDATA            __attribute__((section (".initdata")))
        #define INIT_PRIORITY(X)    __attribute__((init_priority((X))))
        #define NOINLINE            __attribute__((noinline))
        #define NONNULL             __attribute__((nonnull))
        #define NORETURN            __attribute__((noreturn))
        #define REGPARM(X)          __attribute__((regparm(X)))
        #define WARN_UNUSED_RESULT  __attribute__((warn_unused_result))

        #define EXPECT_FALSE(X)     __builtin_expect(!!(X), 0)
        #define EXPECT_TRUE(X)      __builtin_expect(!!(X), 1)

    #if (COMPILER_VERSION < 450)
        #define UNREACHED           __builtin_trap()
    #else
        #define UNREACHED           __builtin_unreachable()
    #endif

        #define ACCESS_ONCE(x)      (*static_cast<volatile typeof(x) *>(&(x)))

#else
        #define COMPILER            "unknown compiler"
        #define COMPILER_VERSION    0
#endif
