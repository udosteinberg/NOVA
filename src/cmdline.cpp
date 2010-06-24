/*
 * Command Line Parser
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#include "cmdline.h"
#include "string.h"

bool Cmdline::dmar;
bool Cmdline::keyb;
bool Cmdline::serial;
bool Cmdline::noacpi;
bool Cmdline::noept;
bool Cmdline::nomp;
bool Cmdline::nospinner;
bool Cmdline::novga;
bool Cmdline::novpid;

struct param
{
    char const *string;
    bool *param;
} params[] INITDATA =
{
    { "dmar",       &Cmdline::dmar      },
    { "keyb",       &Cmdline::keyb      },
    { "serial",     &Cmdline::serial    },
    { "noacpi",     &Cmdline::noacpi    },
    { "noept",      &Cmdline::noept     },
    { "nomp",       &Cmdline::nomp      },
    { "nospinner",  &Cmdline::nospinner },
    { "novga",      &Cmdline::novga     },
    { "novpid",     &Cmdline::novpid    },
};

char *Cmdline::get_arg (char **line)
{
    for (; **line == ' '; ++*line) ;

    if (!**line)
        return 0;

    char *arg = *line;

    for (; **line != ' '; ++*line)
        if (!**line)
            return arg;

    *(*line)++ = 0;

    return arg;
}

void Cmdline::init (char *line)
{
    char *arg;

    while ((arg = get_arg (&line)))
        for (unsigned i = 0; i < sizeof params / sizeof *params; i++)
            if (!strcmp (static_cast<char const *>(phys_ptr (params[i].string)), arg))
                *static_cast<bool *>(phys_ptr (params[i].param)) = true;
}
