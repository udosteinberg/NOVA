/*
 * Board Selection
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#elif defined(BOARD_amlogic_g12b)
#include "board_amlogic_g12b.hpp"

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

#elif defined (BOARD_ti_j721e)
#include "board_ti_j721e.hpp"

#elif defined (BOARD_xilinx_zynq_cg)
#include "board_xilinx_zynq_cg.hpp"

#elif defined (BOARD_xilinx_zynq_u96)
#include "board_xilinx_zynq_u96.hpp"

#elif defined (BOARD_xilinx_zynq_zcu102)
#include "board_xilinx_zynq_zcu102.hpp"

#else
#error "You need to select a valid BOARD (see README.md)!"

#endif
