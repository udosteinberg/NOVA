/*
 * Board Selection
 *
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

#if defined (BOARD_acpi)
#include "board_acpi.hpp"

#elif defined (BOARD_qemu)
#include "board_qemu.hpp"

#elif defined(BOARD_allwinner_a64)
#include "board_allwinner_a64.hpp"

#elif defined(BOARD_amlogic_sm1)
#include "board_amlogic_sm1.hpp"

#elif defined (BOARD_broadcom_bcm2711)
#include "board_broadcom_bcm2711.hpp"

#elif defined(BOARD_hisilicon_hi3660)
#include "board_hisilicon_hi3660.hpp"

#elif defined (BOARD_nvidia_tegrax1)
#include "board_nvidia_tegrax1.hpp"

#elif defined (BOARD_nvidia_tegrax2)
#include "board_nvidia_tegrax2.hpp"

#elif defined (BOARD_nvidia_xavier)
#include "board_nvidia_xavier.hpp"

#elif defined(BOARD_nxp_imx8m)
#include "board_nxp_imx8m.hpp"

#elif defined (BOARD_qualcomm_sdm670)
#include "board_qualcomm_sdm670.hpp"

#elif defined (BOARD_renesas_rcar3)
#include "board_renesas_rcar3.hpp"

#elif defined (BOARD_rockchip_rk3399)
#include "board_rockchip_rk3399.hpp"

#elif defined (BOARD_xilinx_zynq_u96)
#include "board_xilinx_zynq_u96.hpp"

#elif defined (BOARD_xilinx_zynq_zcu102)
#include "board_xilinx_zynq_zcu102.hpp"

// Deprecated BOARD names

#elif defined (BOARD_tegrax1)
#pragma GCC warning "BOARD=tegrax1 is deprecated use BOARD=nvidia_tegrax1 instead!"
#include "board_nvidia_tegrax1.hpp"

#elif defined (BOARD_tegrax2)
#pragma GCC warning "BOARD=tegrax2 is deprecated use BOARD=nvidia_tegrax2 instead!"
#include "board_nvidia_tegrax2.hpp"

#elif defined (BOARD_xavier)
#pragma GCC warning "BOARD=xavier is deprecated use BOARD=nvidia_xavier instead!"
#include "board_nvidia_xavier.hpp"

#elif defined(BOARD_imx8)
#pragma GCC warning "BOARD=imx8 is deprecated use BOARD=nxp_imx8m instead!"
#include "board_nxp_imx8m.hpp"

#elif defined (BOARD_sdm670)
#pragma GCC warning "BOARD=sdm670 is deprecated use BOARD=qualcomm_sdm670 instead!"
#include "board_qualcomm_sdm670.hpp"

#elif defined (BOARD_rpi4)
#pragma GCC warning "BOARD=rpi4 is deprecated use BOARD=broadcom_bcm2711 instead!"
#include "board_broadcom_bcm2711.hpp"

#elif defined (BOARD_rcar)
#pragma GCC warning "BOARD=rcar is deprecated use BOARD=renesas_rcar3 instead!"
#include "board_renesas_rcar3.hpp"

#elif defined (BOARD_u96)
#pragma GCC warning "BOARD=u96 is deprecated use BOARD=xilinx_zynq_u96 instead!"
#include "board_xilinx_zynq_u96.hpp"

#elif defined (BOARD_zcu102)
#pragma GCC warning "BOARD=zcu102 is deprecated use BOARD=xilinx_zynq_zcu102 instead!"
#include "board_xilinx_zynq_zcu102.hpp"

#else
#error "You need to select a valid BOARD (see README.md)!"

#endif
