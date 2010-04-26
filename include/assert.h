/*
 * Assertions
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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
#include "stdio.h"

#ifdef NDEBUG
#define assert(X)   do {} while (0)
#else
#define assert(X)   do {                                                                            \
                        if (EXPECT_FALSE (!(X)))                                                    \
                            panic ("Assertion \"%s\" failed at %s:%d\n", #X, __FILE__, __LINE__);   \
                    } while (0)
#endif
