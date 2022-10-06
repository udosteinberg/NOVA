/*
 * Platform Configuration (PCONFIG)
 *
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

#pragma once

#include "cpuset.hpp"
#include "drng.hpp"

class Pconfig final
{
    public:
        // Encryption Algorithms
        enum class Encrypt : uint8_t
        {
            AES_XTS_128             = BIT (0),  // AES-XTS 128-bit encryption
            AES_XTS_128_INT         = BIT (1),  // AES-XTS 128-bit encryption with integrity
            AES_XTS_256             = BIT (2),  // AES-XTS 256-bit encryption
            AES_XTS_256_INT         = BIT (3),  // AES-XTS 256-bit encryption with integrity
        };

        enum class Status : unsigned
        {
            SUCCESS                 = 0,        // KeyID successfully programmed
            ERR_CMD                 = 1,        // Invalid command
            ERR_ENT                 = 2,        // Insufficient entropy
            ERR_KID                 = 3,        // Invalid key identifier
            ERR_ALG                 = 4,        // Invalid encryption algorithm
            ERR_BUSY                = 5,        // Concurrent key table access
            ERR_UNWRAP              = 7,        // Unwrap failure
        };

        // Sockets whose key table has been TME-MK-programmed, indexed by socket ID
        static inline constinit Cpuset socket_key_prog;

        // Key table defaults to TME-MK-unprogrammed after each CPU reset
        static void init() { socket_key_prog.clr_all(); }

        [[nodiscard]] static auto key_clr (uint16_t k, Encrypt e) { return pconfig (Leaf::KEY_PROGRAM_TME, Key_program { k, Command::CLR_KEY,        e }); }
        [[nodiscard]] static auto key_rnd (uint16_t k, Encrypt e) { return pconfig (Leaf::KEY_PROGRAM_TME, Key_program { k, Command::SET_KEY_RANDOM, e }); }

    private:
        // PCONFIG Leaf Encodings
        enum class Leaf : unsigned
        {
            KEY_PROGRAM_TME         = 0,        // Total Memory Encryption (Multi-Key)
            KEY_PROGRAM_TSE         = 1,        // Total Storage Encryption
            KEY_PROGRAM_TSE_WRAPPED = 2,        // Total Storage Encryption (Wrapped)
        };

        // Key Programming Commands
        enum class Command : uint8_t
        {
            SET_KEY_DIRECT          = 0,        // Tenant Key
            SET_KEY_RANDOM          = 1,        // Ephemeral Key
            CLR_KEY                 = 2,        // TME Default
            NO_ENCRYPT              = 3,        // No Encryption
        };

        // Key Programming Structure
        class alignas (256) Key_program final
        {
            private:
                uint16_t    kid;                // Key Identifier
                Command     cmd;                // Key Programming Command
                Encrypt     enc;                // Encryption Algorithm
                uint32_t    rsvd[15]    { 0 };  // Reserved
                uint64_t    key1[8]     { 0 };  // Data Key
                uint64_t    key2[8]     { 0 };  // Tweak Key
                uint64_t    zero[8]     { 0 };  // Reserved

                // Entropy failure is tolerable because PCONFIG XORs it into a CPU-generated key
                bool add_entropy()
                {
                    switch (enc) {
                        case Encrypt::AES_XTS_128:
                        case Encrypt::AES_XTS_128_INT:
                            return Drng::rand (key1, 2) & Drng::rand (key2, 2);

                        case Encrypt::AES_XTS_256:
                        case Encrypt::AES_XTS_256_INT:
                            return Drng::rand (key1, 4) & Drng::rand (key2, 4);
                    }

                    return false;
                }

            public:
                Key_program (uint16_t k, Command c, Encrypt e) : kid { k }, cmd { c }, enc { e }
                {
                    if (c == Command::SET_KEY_RANDOM)
                        add_entropy();
                }
        };

        static_assert (__is_standard_layout (Key_program) && sizeof (Key_program) == 256);

        [[nodiscard]] static Status pconfig (Leaf l, Key_program const &k)
        {
            unsigned ret;
            asm volatile ("pconfig" : "=a" (ret) : "a" (std::to_underlying (l)), "b" (&k), "m" (k) : "cc");
            return Status { ret };
        }
};
