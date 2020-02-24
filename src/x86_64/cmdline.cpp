/*
 * Command Line Parser
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "cmdline.hpp"
#include "hpt.hpp"
#include "string.hpp"

bool Cmdline::iommu;
bool Cmdline::serial;
bool Cmdline::nodl;
bool Cmdline::nopcid;
bool Cmdline::novpid;

struct Cmdline::param_map Cmdline::map[] =
{
    { "iommu",      &Cmdline::iommu     },
    { "serial",     &Cmdline::serial    },
    { "nodl",       &Cmdline::nodl      },
    { "nopcid",     &Cmdline::nopcid    },
    { "novpid",     &Cmdline::novpid    },
};

char *Cmdline::get_arg (char **line)
{
    for (; **line == ' '; ++*line) ;

    if (!**line)
        return nullptr;

    char *arg = *line;

    for (; **line != ' '; ++*line)
        if (!**line)
            return arg;

    *(*line)++ = 0;

    return arg;
}

void Cmdline::init (mword addr)
{
    char *arg, *line = static_cast<char *>(Hpt::remap (addr));

    while ((arg = get_arg (&line)))
        for (unsigned i = 0; i < sizeof map / sizeof *map; i++)
            if (!strcmp (map[i].arg, arg))
                *map[i].ptr = true;
}
