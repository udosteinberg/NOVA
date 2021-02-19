/*
 * Command Line Parser
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "cmdline.hpp"
#include "extern.hpp"
#include "ptab_hpt.hpp"
#include "string.hpp"

size_t Cmdline::arg_len (char const *&line)
{
    for (; *line == ' '; line++) ;

    char const *p;

    for (p = line; *p && *p != ' '; p++) ;

    return static_cast<size_t>(p - line);
}

void Cmdline::parse (char const *line)
{
    for (size_t len; (len = arg_len (line)); line += len)
        for (unsigned i = 0; i < sizeof (options) / sizeof (*options); i++)
            if (!strncmp (options[i].str, line, len))
                options[i].var = true;
}

void Cmdline::init()
{
    auto addr = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_cl));

    if (addr)
        parse (static_cast<char const *>(Hptp::map (addr)));
}
