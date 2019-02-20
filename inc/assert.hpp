/*
 * Assertions
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

#include "stdio.hpp"

#ifdef DEBUG
#define assert(X)   do {                                                                                            \
                        if (EXPECT_FALSE (!(X)))                                                                    \
                            panic ("Assertion \"%s\" in %s at %s:%d", #X, __PRETTY_FUNCTION__, __FILE__, __LINE__); \
                    } while (0)
#else
#define assert(X)   do { (void) sizeof (X); } while (0)
#endif
