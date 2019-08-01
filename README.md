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

- **make 4.0 or higher**, available from https://ftp.gnu.org/gnu/make/
- **binutils 2.38 or higher**, available from https://ftp.gnu.org/gnu/binutils/
- **gcc 11.4 or higher**, available from https://ftp.gnu.org/gnu/gcc/

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

**Board**                             | **Build Command**                            | **Comments**
------------------------------------- | -------------------------------------------- | --------------------
ACPI Platform                         | `make ARCH=aarch64 BOARD=acpi`               | Default Board
QEMU Virt Platform                    | `make ARCH=aarch64 BOARD=qemu`               | Use `-M virt -smp 4`
Allwinner A64                         | `make ARCH=aarch64 BOARD=allwinner_a64`      | 4 Cortex-A53, GICv2
Amlogic G12B                          | `make ARCH=aarch64 BOARD=amlogic_g12b`       | 2 Cortex-A53, 4 Cortex-A73, GICv2
Amlogic SM1                           | `make ARCH=aarch64 BOARD=amlogic_sm1`        | 4 Cortex-A55, GICv2
Broadcom BCM2711                      | `make ARCH=aarch64 BOARD=broadcom_bcm2711`   | 4 Cortex-A72, GICv2
HiSilicon Hi3660                      | `make ARCH=aarch64 BOARD=hisilicon_hi3660`   | 4 Cortex-A53, 4 Cortex-A73, GICv2
NVIDIA Tegra X1                       | `make ARCH=aarch64 BOARD=nvidia_tegrax1`     | 4 Cortex-A57, GICv2
NVIDIA Tegra X2                       | `make ARCH=aarch64 BOARD=nvidia_tegrax2`     | 4 Cortex-A57, 2 Denver, GICv2, SMMUv2
NVIDIA Xavier                         | `make ARCH=aarch64 BOARD=nvidia_xavier`      | 6 Carmel, GICv2, SMMUv2
NXP i.MX 8M                           | `make ARCH=aarch64 BOARD=nxp_imx8m`          | 4 Cortex-A53, GICv3
Qualcomm Snapdragon 670               | `make ARCH=aarch64 BOARD=qualcomm_sdm670`    | 6 Kryo 360 Silver, 2 Kryo 360 Gold, GICv3, SMMUv2
Renesas R-Car M3                      | `make ARCH=aarch64 BOARD=renesas_rcar3`      | 4 Cortex-A53, 2 Cortex-A57, GICv2
Rockchip RK3399                       | `make ARCH=aarch64 BOARD=rockchip_rk3399`    | 4 Cortex-A53, 2 Cortex-A72, GICv3
Texas Instruments J721E               | `make ARCH=aarch64 BOARD=ti_j721e`           | 2 Cortex-A72, GICv3, SMMUv3
Xilinx Zynq Ultrascale+ MPSoC CG      | `make ARCH=aarch64 BOARD=xilinx_zynq_cg`     | 2 Cortex-A53, GICv2, SMMUv2
Xilinx Zynq Ultrascale+ MPSoC Ultra96 | `make ARCH=aarch64 BOARD=xilinx_zynq_u96`    | 4 Cortex-A53, GICv2, SMMUv2
Xilinx Zynq Ultrascale+ MPSoC ZCU102  | `make ARCH=aarch64 BOARD=xilinx_zynq_zcu102` | 4 Cortex-A53, GICv2, SMMUv2

#### x86 (64bit)

For CPUs with x86 architecture
- Intel VT-x (VMX+EPT) + optionally VT-d
- AMD-V (SVM+NPT)

and boards with Advanced Configuration and Power Interface (ACPI).

**Platform** | **Build Command**  | **Comments**
------------ | -------------------| --------------------
x86_64       | `make ARCH=x86_64` | 64bit or 32bit VMs

##### Control-Flow Enforcement Technology (CET)

NOVA can be built with support for control-flow protection. Because
control-flow protected binaries require a CPU with CET support and because
of the resulting performance overhead, CFP is disabled by default.
Protection features can be enabled at build time as follows:

**Build Command**             | **Feature Level**
------------------------------| -----------------
`make ARCH=x86_64 CFP=none`   | No control-flow protection (Default)
`make ARCH=x86_64 CFP=branch` | CET indirect branch tracking (IBT)
`make ARCH=x86_64 CFP=return` | CET supervisor shadow stacks (SSS)
`make ARCH=x86_64 CFP=full`   | CET IBT and CET SSS

##### Trusted Execution Technology (TXT)

On TXT-enabled platforms, NOVA performs a measured launch to establish a
Dynamic Root of Trust for Measurement (DRTM) if an SINIT Authenticated Code
Module (ACM) matching the platform is present in TXT memory.

The SINIT ACM is typically loaded into TXT memory
- on server platforms: by the firmware
- on client platforms: by the bootloader

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
Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.

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
