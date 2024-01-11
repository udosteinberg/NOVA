/*
 * Command Line Parser
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "types.hpp"

class Cmdline final
{
    public:
        // Command line parameters must be in a measured section
        SEC_HASH static inline bool insecure { false };
        SEC_HASH static inline bool noccst   { false };
        SEC_HASH static inline bool nodl     { false };
        SEC_HASH static inline bool nopcid   { false };
        SEC_HASH static inline bool nosmmu   { false };
        SEC_HASH static inline bool nouart   { false };
        SEC_HASH static inline bool novpid   { false };

        static void init();

    private:
        static constexpr struct
        {
            char const *    str;
            bool &          var;
        } options[]
        {
            { "insecure",   insecure    },
            { "noccst",     noccst      },
            { "nodl",       nodl        },
            { "nopcid",     nopcid      },
            { "nosmmu",     nosmmu      },
            { "nouart",     nouart      },
            { "novpid",     novpid      },
        };

        static inline size_t arg_len (char const *&);

        static void parse (char const *);
};
