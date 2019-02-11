# NOVA Microhypervisor

This is the source code for the NOVA microhypervisor.

The NOVA microhypervisor combines microkernel and hypervisor functionality
and provides an extremely small trusted computing base for user applications
and virtual machines running on top of it. The microhypervisor implements a
capability-based authorization model and provides basic mechanisms for
virtualization, spatial and temporal separation, scheduling, communication,
and management of platform resources.

NOVA can be used with a multi-server environment that implements additional
operating-system services in user mode, such as device drivers, protocol
stacks, and policies. On machines with hardware virtualization features,
multiple unmodified guest operating systems can run concurrently on top of
the microhypervisor.

**This code is experimental and not feature complete. If it breaks, you get
  to keep both pieces.**

## Building

### Required Tools

The following tools are needed to compile the source code:

- **make 3.81 or higher**, available from https://ftp.gnu.org/gnu/make/
- **binutils 2.38 or higher**, available from https://ftp.gnu.org/gnu/binutils/
- **gcc 7.5 or higher**, available from https://ftp.gnu.org/gnu/gcc/

### Build Environment

The build environment can be customized permanently in `Makefile.conf` or
ad hoc by passing the applicable `ARCH`, `BOARD` and `PREFIX_` variables to
the invocation of `make` as described below.

- `PREFIX_aarch64` sets the path for an **ARMv8-A** cross-toolchain
- `PREFIX_x86_64` sets the path for an **x86 (64bit)** cross-toolchain

For example, if the ARMv8-A cross-toolchain is located at
```
/opt/aarch64-linux/bin/aarch64-linux-gcc
/opt/aarch64-linux/bin/aarch64-linux-as
/opt/aarch64-linux/bin/aarch64-linux-ld
```

then set `PREFIX_aarch64=/opt/aarch64-linux/bin/aarch64-linux-`

### Supported Architectures

#### ARMv8-A (64bit)

For CPUs with ARMv8-A architecture and boards with
- either Advanced Configuration and Power Interface (ACPI)
- or Flattened Device Tree (FDT)

#### x86 (64bit)

For CPUs with x86 architecture
- Intel VT-x (VMX+EPT) + optionally VT-d
- AMD-V (SVM+NPT)

and boards with Advanced Configuration and Power Interface (ACPI).

**Platform** | **Build Command**  | **Comments**
------------ | -------------------| --------------------
x86_64       | `make ARCH=x86_64` | 64bit or 32bit VMs

## Booting

See the NOVA interface specification in the `doc` directory for details
regarding booting the NOVA microhypervisor.

## License

The NOVA source code is licensed under the **GPL version 2**.

```
Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
Economic rights: Technische Universitaet Dresden (Germany)

Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.

NOVA is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

NOVA is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License version 2 for more details.
```

## Contact

Feedback and comments should be sent to udo@hypervisor.org
