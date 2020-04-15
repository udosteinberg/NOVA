/*
 * Command Line Parser
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "types.hpp"

class Cmdline
{
    public:
        static inline bool serial   { false };
        static inline bool nodl     { false };
        static inline bool nopcid   { false };
        static inline bool nosmmu   { false };
        static inline bool noturbo  { false };
        static inline bool novpid   { false };

        static void parse (char const *);

    private:
        static constexpr struct
        {
            char const *    str;
            bool &          var;
        } options[] =
        {
            { "serial",     serial      },
            { "nodl",       nodl        },
            { "nopcid",     nopcid      },
            { "nosmmu",     nosmmu      },
            { "noturbo",    noturbo     },
            { "novpid",     novpid      },
        };

        static inline size_t arg_len (char const *&);
};
