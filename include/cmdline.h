/*
 * Command Line Parser
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "types.h"

class Cmdline
{
    private:
        INIT
        static char *get_arg (char **line);

    public:
        static bool dmar;
        static bool keyb;
        static bool serial;
        static bool noacpi;
        static bool noept;
        static bool nomp;
        static bool nospinner;
        static bool novga;
        static bool novpid;

        ALWAYS_INLINE
        static inline void *phys_ptr (void const *ptr)
        {
            extern char OFFSET;
            return reinterpret_cast<void *>(reinterpret_cast<mword>(ptr) -
                                            reinterpret_cast<mword>(&OFFSET));
        }

        INIT
        static void init (char *line);
};
