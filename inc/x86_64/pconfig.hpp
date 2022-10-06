/*
 * Platform Configuration (PCONFIG)
 *
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

#include "drng.hpp"

class Pconfig final
{
    private:
        // PCONFIG Leaf Encodings
        enum class Leaf : unsigned
        {
            MKTME_KEY_PROGRAM   = 0,
        };

        // Key Programming Commands
        enum class Command : uint8
        {
            SET_KEY_DIRECT      = 0,
            SET_KEY_RANDOM      = 1,
            CLR_KEY             = 2,
            NO_ENCRYPT          = 3,
        };

        // Encryption Algorithms
        enum class Encrypt : uint8
        {
            AES_XTS_128         = BIT (0),
            AES_XTS_256         = BIT (2),
        };

        // Key Programming Structure
        class alignas (256) Mktme_key_program final
        {
            private:
                uint16  kid;                    // Key Identifier
                Command cmd;                    // Key Programming Command
                Encrypt enc;                    // Encryption Algorithm
                uint32  rsvd[15]    { 0 };      // Reserved
                uint64  key1[8]     { 0 };      // Data Key
                uint64  key2[8]     { 0 };      // Tweak Key
                uint64  zero[8]     { 0 };      // Reserved

            public:
                Mktme_key_program (uint16 k, Command c, Encrypt e) : kid (k), cmd (c), enc (e)
                {
                    if (c != Command::SET_KEY_RANDOM)
                        return;

                    switch (e) {                // Entropy
                        case Encrypt::AES_XTS_128: Drng::rand (key1, 2); Drng::rand (key2, 2); break;
                        case Encrypt::AES_XTS_256: Drng::rand (key1, 4); Drng::rand (key2, 4); break;
                    }
                }
        };

        static_assert (__is_standard_layout (Mktme_key_program) && sizeof (Mktme_key_program) == 256);

        [[nodiscard]] static bool pconfig (Mktme_key_program const &p)
        {
            uint64 ret;
            asm volatile ("pconfig" : "=a" (ret) : "a" (std::to_underlying (Leaf::MKTME_KEY_PROGRAM)), "b" (&p), "m" (p) : "cc");
            return !ret;
        }

    public:
        static auto key_clr (uint16 k) { return pconfig (Mktme_key_program { k, Command::CLR_KEY,        Encrypt::AES_XTS_128 }); }
        static auto key_rnd (uint16 k) { return pconfig (Mktme_key_program { k, Command::SET_KEY_RANDOM, Encrypt::AES_XTS_128 }); }
};
