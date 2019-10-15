/*
 * Generic Console
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include <stdarg.h>
#include "initprio.hpp"
#include "list.hpp"
#include "spinlock.hpp"

class Console : public List<Console>
{
    private:
        enum class Mode
        {
            FLAGS       = 0,
            WIDTH       = 1,
            PRECS       = 2,
        };

        enum Flags
        {
            SIGNED      = BIT (0),
            ALT_FORM    = BIT (1),
            ZERO_PAD    = BIT (2),
        };

        static inline Console *dormant { nullptr };
        static inline Console *enabled { nullptr };
        static inline Spinlock lock;

        virtual void outc (char) = 0;

        static void putc (char c)
        {
            for (auto e { enabled }; e; e = e->next)
                e->outc (c);
        }

        static void print_num (uint64, unsigned, unsigned, unsigned);
        static void print_str (char const *, unsigned, unsigned);

        FORMAT (1,0)
        static void vprintf (char const *, va_list);

    protected:
        Console() : List (dormant) {}

        virtual void init() const {};
        virtual bool fini() const { return false; }

        void enable()
        {
            remove (dormant);
            insert (enabled);
        }

        void disable()
        {
            remove (enabled);
            insert (dormant);
        }

    public:
        FORMAT (1,2)
        static void print (char const *, ...);

        [[noreturn]] FORMAT (1,2)
        static void panic (char const *, ...);

        static void flush()
        {
            for (Console *e { enabled }, *n; e; e = n) {

                n = e->next;

                if (e->fini())
                    e->disable();
            }
        }
};
