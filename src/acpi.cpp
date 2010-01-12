/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2005-2010, Udo Steinberg <udo@hypervisor.org>
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

#include "acpi.h"
#include "acpi_dmar.h"
#include "acpi_fadt.h"
#include "acpi_madt.h"
#include "acpi_mcfg.h"
#include "acpi_rsdp.h"
#include "acpi_rsdt.h"
#include "assert.h"
#include "cmdline.h"
#include "gsi.h"
#include "io.h"
#include "ptab.h"
#include "stdio.h"
#include "x86.h"

Paddr Acpi::dmar, Acpi::fadt, Acpi::madt, Acpi::mcfg, Acpi::rsdt, Acpi::xsdt;

Acpi_gas Acpi::pm1a_sts;
Acpi_gas Acpi::pm1b_sts;
Acpi_gas Acpi::pm1a_ena;
Acpi_gas Acpi::pm1b_ena;
Acpi_gas Acpi::pm1a_cnt;
Acpi_gas Acpi::pm1b_cnt;
Acpi_gas Acpi::pm2_cnt;
Acpi_gas Acpi::pm_tmr;
Acpi_gas Acpi::reset_reg;

Acpi::Mode  Acpi::mode = PIC;
uint32      Acpi::feature;
uint32      Acpi::smi_cmd;
unsigned    Acpi::gsi;
unsigned    Acpi::irq;
uint8       Acpi::enable_val;
uint8       Acpi::reset_val;

bool        Acpi_table_madt::sci_overridden = false;

void Acpi::setup_sci()
{
    gsi = Gsi::irq_to_gsi (irq);

    if (!Acpi_table_madt::sci_overridden) {
        Acpi_intr_override sci_override;
        sci_override.bus = 0;
        sci_override.irq = static_cast<uint8>(irq);
        sci_override.gsi = gsi;
        sci_override.flags.pol = Acpi_inti::POL_CONFORMING;
        sci_override.flags.trg = Acpi_inti::TRG_CONFORMING;
        Acpi_table_madt::parse_intr_override (&sci_override);
    }
}

void Acpi::enable()
{
    trace (TRACE_ACPI, "ACPI: GSI:%#x %s", gsi, "APIC" + (mode == PIC));

    write (PM1_ENA, PM1_ENA_PWRBTN | PM1_ENA_GBL);

    Gsi::unmask (gsi);

    if (Cmdline::noacpi || !smi_cmd || !enable_val)
        return;

    Io::out (smi_cmd, enable_val);
    while ((read (PM1_CNT) & PM1_CNT_SCI_EN) == 0)
        pause();
}

void Acpi::delay (unsigned ms)
{
    unsigned cnt = timer_frequency * ms / 1000;
    unsigned val = read (PM_TMR);

    while ((read (PM_TMR) - val) % (1ul << 24) < cnt)
        pause();
}

void Acpi::reset()
{
//  if (feature & 0x400)
        write (RESET, reset_val);
}

void Acpi::setup()
{
    Acpi_rsdp::parse();

    if (xsdt)
        static_cast<Acpi_table_rsdt *>(Ptab::master()->remap (xsdt))->parse (xsdt, sizeof (uint64));
    else if (rsdt)
        static_cast<Acpi_table_rsdt *>(Ptab::master()->remap (rsdt))->parse (rsdt, sizeof (uint32));

    if (fadt)
        static_cast<Acpi_table_fadt *>(Ptab::master()->remap (fadt))->parse();
    if (madt)
        static_cast<Acpi_table_madt *>(Ptab::master()->remap (madt))->parse();
    if (mcfg)
        static_cast<Acpi_table_mcfg *>(Ptab::master()->remap (mcfg))->parse();
    if (dmar)
        static_cast<Acpi_table_dmar *>(Ptab::master()->remap (dmar))->parse();

    setup_sci();

    Gsi::init();

    enable();
}

unsigned Acpi::read (Register reg)
{
    switch (reg) {
        case PM1_STS:
            return hw_read (&pm1a_sts) | hw_read (&pm1b_sts);
        case PM1_ENA:
            return hw_read (&pm1a_ena) | hw_read (&pm1b_ena);
        case PM1_CNT:
            return hw_read (&pm1a_cnt) | hw_read (&pm1b_cnt);
        case PM2_CNT:
            return hw_read (&pm2_cnt);
        case PM_TMR:
            return hw_read (&pm_tmr);
        case RESET:
            break;
    }

    return 0;
}

void Acpi::write (Register reg, unsigned val)
{
    // XXX: Spec requires that certain bits be preserved.

    switch (reg) {
        case PM1_STS:
            hw_write (&pm1a_sts, val);
            hw_write (&pm1b_sts, val);
            break;
        case PM1_ENA:
            hw_write (&pm1a_ena, val);
            hw_write (&pm1b_ena, val);
            break;
        case PM1_CNT:
            hw_write (&pm1a_cnt, val);
            hw_write (&pm1b_cnt, val);
            break;
        case PM2_CNT:
            hw_write (&pm2_cnt, val);
            break;
        case PM_TMR:                    // read-only
            break;
        case RESET:
            hw_write (&reset_reg, val);
            break;
    }
}

unsigned Acpi::hw_read (Acpi_gas *gas)
{
    if (!gas->bits)     // Register not implemented
        return 0;

    if (gas->asid == Acpi_gas::IO) {
        switch (gas->bits) {
            case 8:
                return Io::in<uint8>(static_cast<unsigned>(gas->addr));
            case 16:
                return Io::in<uint16>(static_cast<unsigned>(gas->addr));
            case 32:
                return Io::in<uint32>(static_cast<unsigned>(gas->addr));
        }
    }

    panic ("Unimplemented ASID %u\n", gas->asid);
}

void Acpi::hw_write (Acpi_gas *gas, unsigned val)
{
    if (!gas->bits)     // Register not implemented
        return;

    if (gas->asid == Acpi_gas::IO) {
        switch (gas->bits) {
            case 8:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint8>(val));
                return;
            case 16:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint16>(val));
                return;
            case 32:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint32>(val));
                return;
        }
    }

    panic ("Unimplemented ASID %u\n", gas->asid);
}

void Acpi::interrupt()
{
    write (PM1_STS, read (PM1_STS));
}
