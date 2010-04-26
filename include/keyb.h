/*
 * Keyboard
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
#include "x86.h"

class Keyb
{
    private:
        enum
        {
            INP = 0x60,
            OUT = 0x60,
            STS = 0x64,
            CMD = 0x64
        };

        enum
        {
            STS_OUTB        = 1u << 0,  // Output Buffer Full
            STS_INPB        = 1u << 1,  // Input  Buffer Full
            STS_SYSF        = 1u << 2,  // System Flag
            STS_CD          = 1u << 3,  // Command/Data Written
            STS_INHB        = 1u << 4,  // Inhibit
            STS_AUXB        = 1u << 5,  // Auxilary
            STS_TIME        = 1u << 6,  // Timeout
            STS_PARE        = 1u << 7,  // Parity Error

            CCB_KIE         = 1u << 0,  // Keyboard Interrupt Enabled
            CCB_MIE         = 1u << 1,  // Mouse Interrupt Enabled
            CCB_SYSF        = 1u << 2,  // System Flag
            CCB_IGNLK       = 1u << 3,  // Ignore Keyboard Lock
            CCB_KD          = 1u << 4,  // Keyboard Disabled
            CCB_MD          = 1u << 5,  // Mouse Disabled
            CCB_XLATE       = 1u << 6,  // Translate Scancodes
        };

        enum Cmd
        {
            CMD_RD_CCB          = 0x20,
            CMD_WR_CCB          = 0x60,
            CMD_DIS_AUX         = 0xa7,
            CMD_ENA_AUX         = 0xa8,
            CMD_AUX_SELFTEST    = 0xa9,
            CMD_CTL_SELFTEST    = 0xaa,
            CMD_IFC_SELFTEST    = 0xab,
            CMD_DIS_KEY         = 0xad,
            CMD_ENA_KEY         = 0xae,
            CMD_READ_TEST       = 0xe0,
            CMD_RESET           = 0xfe
        };

        enum
        {
            KEYB_SCAN_CODE      = 0xf0,
            KEYB_READ_ID        = 0xf2,
            KEYB_ACK            = 0xfa,
            KEYB_RESEND         = 0xfe,
            KEYB_RESET          = 0xff
        };

        ALWAYS_INLINE
        static inline unsigned status()
        {
            return Io::in<uint8>(STS);
        }

        ALWAYS_INLINE
        static inline void wait_recv()
        {
            while ((status() & STS_OUTB) == 0)
                pause();
        }

        ALWAYS_INLINE
        static inline void wait_send()
        {
            while ((status() & STS_INPB) != 0)
                pause();
        }

        ALWAYS_INLINE
        static inline unsigned input()
        {
            return Io::in<uint8>(INP);
        }

        ALWAYS_INLINE
        static inline unsigned read_keyb()
        {
            wait_recv();
            return input();
        }

        ALWAYS_INLINE
        static inline void send_ctrl (Cmd cmd)
        {
            wait_send();
            Io::out<uint8>(CMD, cmd);
        }

        ALWAYS_INLINE
        static inline void send_keyb (unsigned val)
        {
            wait_send();
            Io::out (OUT, static_cast<uint8>(val));
        }

    public:
        static unsigned const irq = 1;
        static unsigned gsi;

        INIT
        static void init();

        static void interrupt();
};
