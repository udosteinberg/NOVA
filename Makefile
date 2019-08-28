#
# Makefile
#
# Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
# Economic rights: Technische Universitaet Dresden (Germany)
#
# Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
# Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
#
# This file is part of the NOVA microhypervisor.
#
# NOVA is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# NOVA is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License version 2 for more details.
#

# The following file should be customized for your environment
-include Makefile.conf

# Defaults
ARCH	?= aarch64
BOARD	?= qemu
COMP	?= gcc

# Tools
INSTALL	:= install
MKDIR	:= mkdir
ifeq ($(COMP),gcc)
CC	:= $(PREFIX-$(ARCH))gcc
LD	:= $(PREFIX-$(ARCH))ld
OC	:= $(PREFIX-$(ARCH))objcopy
SZ	:= $(PREFIX-$(ARCH))size
else
$(error $(COMP) is not a valid compiler type)
endif
H2E	:= $(H2E-$(ARCH))
H2B	:= $(H2B-$(ARCH))
RUN	:= $(RUN-$(ARCH))

# Directories
SRC_DIR	:= src/$(ARCH) src
INC_DIR	:= inc/$(ARCH) inc
BLD_DIR	?= build-$(ARCH)

# Patterns
PAT_OBJ	:= $(BLD_DIR)/$(ARCH)-%.o

# Files
SRC	:= hypervisor.ld $(sort $(notdir $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.S)))) $(sort $(notdir $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.cpp))))
OBJ	:= $(patsubst %.ld,$(PAT_OBJ), $(patsubst %.S,$(PAT_OBJ), $(patsubst %.cpp,$(PAT_OBJ), $(SRC))))
OBJ_DEP	:= $(OBJ:%.o=%.d)

ifeq ($(ARCH),aarch64)
HYP	:= $(BLD_DIR)/$(ARCH)-$(BOARD)-hypervisor
else
HYP	:= $(BLD_DIR)/$(ARCH)-hypervisor
endif
ELF	:= $(HYP).elf
BIN	:= $(HYP).bin

# Messages
ifneq ($(findstring s,$(MAKEFLAGS)),)
message = @echo $(1) $(2)
endif

# Tool check
tools = $(if $(shell command -v $($(1)) 2>/dev/null),, $(error Missing $(1)=$($(1))))

# Feature check
check = $(shell if $(CC) $(1) -Werror -c -xc++ /dev/null -o /dev/null >/dev/null 2>&1; then echo "$(1)"; fi)

# Version check
gitrv = $(shell (git rev-parse HEAD 2>/dev/null || echo 0) | cut -c1-7)

# Search path
VPATH	:= $(SRC_DIR)

# Optimization options
DFLAGS	:= -MP -MMD -pipe
OFLAGS	:= -Os
ifeq ($(ARCH),aarch64)
AFLAGS	:= -march=armv8-a -mcmodel=large -mgeneral-regs-only $(call check,-mno-outline-atomics) -mstrict-align
DEFINES	+= BOARD_$(BOARD)
else ifeq ($(ARCH),x86_64)
AFLAGS	:= -m64 -march=core2 -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mpreferred-stack-boundary=4
else
$(error $(ARCH) is not a valid architecture)
endif

# Preprocessor options
PFLAGS	:= $(addprefix -D, $(DEFINES)) $(addprefix -I, $(INC_DIR))

# Language options
FFLAGS	:= $(or $(call check,-std=gnu++20), $(call check,-std=gnu++17), $(call check,-std=gnu++14), $(call check,-std=gnu++11))
FFLAGS	+= -fdata-sections -ffreestanding -ffunction-sections -fomit-frame-pointer -funit-at-a-time
FFLAGS	+= -fno-asynchronous-unwind-tables -fno-exceptions -fno-rtti
FFLAGS	+= $(call check,-fdiagnostics-color=auto)
FFLAGS	+= $(call check,-fno-pie)
FFLAGS	+= $(call check,-fno-stack-protector)
FFLAGS	+= $(call check,-freg-struct-return)
FFLAGS	+= $(call check,-freorder-blocks)
FFLAGS	+= $(call check,-fvisibility-inlines-hidden)

# Warning options
WFLAGS	:= -Wall -Wextra -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wformat=2 -Wmissing-format-attribute -Wmissing-noreturn -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wwrite-strings
WFLAGS	+= -Wctor-dtor-privacy -Wno-non-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wsign-promo
WFLAGS	+= $(call check,-Wlogical-op)
WFLAGS	+= $(call check,-Wstrict-null-sentinel)
WFLAGS	+= $(call check,-Wstrict-overflow=5)
WFLAGS	+= $(call check,-Wsuggest-final-methods)
WFLAGS	+= $(call check,-Wsuggest-final-types)
WFLAGS	+= $(call check,-Wsuggest-override)
WFLAGS	+= $(call check,-Wvolatile-register-var)
WFLAGS	+= $(call check,-Wzero-as-null-pointer-constant)

ifeq ($(ARCH),aarch64)
WFLAGS	+= $(call check,-Wpedantic)
endif

# Compiler flags
SFLAGS	:= $(PFLAGS) $(DFLAGS) $(AFLAGS) $(FFLAGS)
CFLAGS	:= $(PFLAGS) $(DFLAGS) $(AFLAGS) $(FFLAGS) $(OFLAGS) $(WFLAGS)

# Linker flags
LFLAGS	:= --defsym=GIT_VER=0x$(call gitrv) --gc-sections --warn-common -static -n -s -T

# Rules
$(HYP):			$(OBJ)
			$(call message,LNK,$@)
			$(LD) $(LFLAGS) $^ -o $@

$(ELF):			$(HYP)
			$(call message,ELF,$@)
			$(H2E) $< $@

$(BIN):			$(HYP)
			$(call message,BIN,$@)
			$(H2B) $< $@

$(PAT_OBJ):		%.ld $(MAKEFILE_LIST)
			$(call message,PRE,$@)
			$(CC) $(SFLAGS) -xassembler-with-cpp -E -P -MT $@ $< -o $@

$(PAT_OBJ):		%.S $(MAKEFILE_LIST)
			$(call message,ASM,$@)
			$(CC) $(SFLAGS) -c $< -o $@

$(PAT_OBJ):		%.cpp $(MAKEFILE_LIST)
			$(call message,CMP,$@)
			$(CC) $(CFLAGS) -c $< -o $@

Makefile.conf:
			$(call message,CFG,$@)
			@cp $@.example $@

$(OBJ):			| dir_bld tool_cc

.PHONY:			clean install run dir_bld tool_cc

clean:
			$(call message,CLN,$@)
			$(RM) $(OBJ) $(HYP) $(ELF) $(BIN) $(OBJ_DEP)

install:		$(ELF)
			$(call message,INS,$^)
			$(INSTALL) -m 644 $^ $(INS_DIR)
			@$(SZ) $<

run:			$(ELF)
			$(RUN) $<

dir_bld:
			@$(MKDIR) -p $(BLD_DIR)

tool_cc:
			$(call tools,CC)

# Include Dependencies
ifneq ($(MAKECMDGOALS),clean)
-include		$(OBJ_DEP)
endif
