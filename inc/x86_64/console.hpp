/*
 * Generic Console
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include <stdarg.h>
#include "compiler.hpp"
#include "initprio.hpp"
#include "spinlock.hpp"

class Console
{
    private:
        enum
        {
            MODE_FLAGS      = 0,
            MODE_WIDTH      = 1,
            MODE_PRECS      = 2,
            FLAG_SIGNED     = 1UL << 0,
            FLAG_ALT_FORM   = 1UL << 1,
            FLAG_ZERO_PAD   = 1UL << 2,
        };

        Console *next { nullptr };

        static Console *list;
        static Spinlock lock;

        virtual void outc (char) = 0;
        virtual bool fini() = 0;

        static void putc (char c)
        {
            for (Console *con = list; con; con = con->next)
                con->outc (c);
        }

        static void print_num (uint64, unsigned, unsigned, unsigned);
        static void print_str (char const *, unsigned, unsigned);

        FORMAT (1,0)
        static void vprintf (char const *, va_list);

    protected:
        NOINLINE
        void enable()
        {
            Console **ptr;
            for (ptr = &list; *ptr; ptr = &(*ptr)->next) ;
            *ptr = this;
        }

        NOINLINE
        void disable()
        {
            Console **ptr;
            for (ptr = &list; *ptr && *ptr != this; ptr = &(*ptr)->next) ;
            if (*ptr)
                *ptr = (*ptr)->next;
        }

    public:
        FORMAT (1,2)
        static void print (char const *, ...);

        FORMAT (1,2) NORETURN
        static void panic (char const *, ...);

        static void flush()
        {
            for (Console *con = list; con; con = con->next)
                if (con->fini())
                    con->disable();
        }
};
