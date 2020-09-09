/*
 * Command Line Parser
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

void Cmdline::parse (char const *line)
{
    for (size_t len; (len = arg_len (line)); line += len)
        for (unsigned i { 0 }; i < sizeof (options) / sizeof (*options); i++)
            if (!strncmp (options[i].str, line, len) && !options[i].str[len]) {
                options[i].var = true;
                break;
            }
}

void Cmdline::init (Paddr addr)
{
    parse (static_cast<char const *>(Hpt::remap (addr)));
}
