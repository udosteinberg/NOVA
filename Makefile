#
# Makefile
#
# Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
# Economic rights: Technische Universitaet Dresden (Germany)
#
# Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
# Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

-include Makefile.conf

# Defaults
ARCH	?= x86_64
BOARD	?= acpi
COMP	?= gcc

# Tools
INSTALL	?= install -m 644
MKDIR	?= mkdir -p
ifeq ($(COMP),gcc)
HST_CC	?= g++
TGT_CC	:= $(PREFIX_$(ARCH))g++
TGT_LD	:= $(PREFIX_$(ARCH))ld
TGT_OC	:= $(PREFIX_$(ARCH))objcopy
TGT_SZ	:= $(PREFIX_$(ARCH))size
else
$(error $(COMP) is not a valid compiler type)
endif
H2E	:= $(H2E_$(ARCH))
H2B	:= $(H2B_$(ARCH))
RUN	:= $(RUN_$(ARCH))

# In-place editing works differently between GNU/BSD sed
SEDI	:= $(shell if sed --version 2>/dev/null | grep -q GNU; then echo "sed -i"; else echo "sed -i ''"; fi)

# Directories
CMD_DIR	:= cmd
SRC_DIR	:= src
INC_DIR	:= include
BLD_DIR	?= build-$(ARCH)

# Patterns
PAT_CMD	:= $(BLD_DIR)/%
PAT_OBJ	:= $(BLD_DIR)/$(ARCH)-%.o

# Files
MFL	:= $(MAKEFILE_LIST)
SRC	:= hypervisor.ld $(sort $(notdir $(foreach d,$(SRC_DIR),$(wildcard $(d)/*.S)))) $(sort $(notdir $(foreach d,$(SRC_DIR),$(wildcard $(d)/*.cpp))))
OBJ	:= $(patsubst %.ld,$(PAT_OBJ), $(patsubst %.S,$(PAT_OBJ), $(patsubst %.cpp,$(PAT_OBJ), $(SRC))))
OBJ_DEP	:= $(OBJ:%.o=%.d)

HYP	:= $(BLD_DIR)/$(ARCH)-nova
ELF	:= $(HYP).elf
BIN	:= $(HYP).bin

# Messages
ifneq ($(findstring s,$(MAKEFLAGS)),)
message = @echo $(1) $(2)
endif

# Tool check
tools = $(if $(shell command -v $($(1)) 2>/dev/null),, $(error Missing $(1)=$($(1)) *** Configure it in Makefile.conf (see Makefile.conf.example)))

# Feature check
check = $(shell if $(TGT_CC) $(1) -Werror -c -xc++ /dev/null -o /dev/null >/dev/null 2>&1; then echo "$(1)"; fi)

# Version check
gitrv = $(shell (git rev-parse HEAD 2>/dev/null || echo 0) | cut -c1-7)

# Search path
VPATH	:= $(SRC_DIR)

# Optimization options
DFLAGS	:= -MP -MMD -pipe
OFLAGS	:= -Os
ifeq ($(ARCH),x86_64)
MFLAGS	:= -Wa,--divide,--noexecstack -march=x86-64-v2 -mcmodel=kernel -mgeneral-regs-only -mno-red-zone
else
$(error $(ARCH) is not a valid architecture)
endif

# Preprocessor options
PFLAGS	:= $(addprefix -D, $(DEFINES))
PFLAGS	+= $(addprefix -I, $(INC_DIR))

# Language options
FFLAGS	:= $(or $(call check,-std=gnu++26), $(call check,-std=gnu++23))
FFLAGS	+= -ffreestanding -fdata-sections -ffunction-sections -fdiagnostics-color=auto -fno-asynchronous-unwind-tables -fno-exceptions -fno-pic -fno-rtti -fno-stack-protector -fno-use-cxa-atexit -fomit-frame-pointer
# Language options added in gcc-12
FFLAGS	+= $(call check,-ftrivial-auto-var-init=uninitialized)

# Warning options
WFLAGS	:= -Wall -Wextra -Walloca -Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wdisabled-optimization -Wduplicated-branches -Wduplicated-cond -Wenum-conversion -Wextra-semi -Wformat=2 -Wlogical-op -Wmismatched-tags -Wmissing-format-attribute -Wmissing-noreturn -Wmultichar -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wpacked -Wpointer-arith -Wredundant-decls -Wredundant-tags -Wregister -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wsuggest-override -Wvirtual-inheritance -Wvolatile -Wvolatile-register-var -Wwrite-strings -Wzero-as-null-pointer-constant
# Warning options added in gcc-12
WFLAGS	+= $(call check,-Wbidi-chars=any)
# Warning options added in gcc-14
WFLAGS	+= $(call check,-Wnrvo)
# Warning options added in gcc-15
WFLAGS	+= $(call check,-Wleading-whitespace=spaces)
WFLAGS	+= $(call check,-Wtrailing-whitespace=any)

ifeq ($(ARCH),aarch64)
WFLAGS	+= $(call check,-Wpedantic)
endif

# Compiler flags
CFLAGS	:= $(PFLAGS) $(DFLAGS) $(MFLAGS) $(FFLAGS) $(OFLAGS) $(WFLAGS)

# Linker flags
LFLAGS	:= --defsym=GIT_VER=0x$(call gitrv) --gc-sections --warn-common -static -n -s -T

# Rules
$(HYP):			$(OBJ)
			$(call message,LNK,$@)
			$(TGT_LD) $(LFLAGS) $^ -o $@

$(ELF):			$(HYP)
			$(call message,ELF,$@)
			$(H2E) $< $@

$(BIN):			$(HYP)
			$(call message,BIN,$@)
			$(H2B) $< $@

$(PAT_OBJ):		%.ld
			$(call message,PRE,$@)
			$(TGT_CC) $(CFLAGS) -xassembler-with-cpp -E -P -MT $@ $< -o $@
			@$(SEDI) 's|$<|$(notdir $<)|' $(@:%.o=%.d)

$(PAT_OBJ):		%.S
			$(call message,ASM,$@)
			$(TGT_CC) $(CFLAGS) -c $< -o $@
			@$(SEDI) 's|$<|$(notdir $<)|' $(@:%.o=%.d)

$(PAT_OBJ):		%.cpp
			$(call message,CXX,$@)
			$(TGT_CC) $(CFLAGS) -c $< -o $@
			@$(SEDI) 's|$<|$(notdir $<)|' $(@:%.o=%.d)

$(PAT_CMD):		$(CMD_DIR)/%.cpp
			$(call message,CMD,$@)
			$(HST_CC) $< -o $@

$(BLD_DIR):
			$(call message,DIR,$@)
			@$(MKDIR) $@

Makefile.conf:
			$(call message,CFG,$@)
			@cp $@.example $@

$(OBJ):			$(MFL) | $(BLD_DIR) tool_tgt_cc

# Zap old-fashioned suffixes
.SUFFIXES:

.PHONY:			clean install run tool_hst_cc tool_tgt_cc

clean:
			$(call message,CLN,$@)
			$(RM) $(OBJ) $(HYP) $(ELF) $(BIN) $(OBJ_DEP)

install:		$(foreach d,$(INS_DIR),install-to-$(subst :,@,$(d)))
			@echo "Section Sizes for $(HYP)"
			@$(TGT_SZ) $(HYP)

run:			$(ELF)
			$(RUN) $<

tool_hst_cc:
			$(call tools,HST_CC)

tool_tgt_cc:
			$(call tools,TGT_CC)

# Create a rule for each install directory
define INSTALL_RULE =
install-to-$(subst :,@,$(2)): $(1)
			$(call message,INS,$(1) =\> $(2))
			$(INSTALL) $(1) $(2)
endef
$(foreach d,$(INS_DIR),$(eval $(call INSTALL_RULE,$(HYP),$(d))))

# Include Dependencies
ifneq ($(MAKECMDGOALS),clean)
-include		$(OBJ_DEP)
endif
