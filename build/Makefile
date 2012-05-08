#
# Makefile
#
# Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
# Economic rights: Technische Universitaet Dresden (Germany)
#
# Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

ARCH		?= x86_32
CC		:= gcc
ECHO		:= echo
INSTALL		:= install
LD		:= ld
RM		:= rm -f
SIZE		:= size

SRC_DIR		:= ../src
INC_DIR		:= ../include
INS_DIR		?= /boot/tftp/nova
TARGET		:= hypervisor-$(ARCH)

SRC		:= hypervisor.ld $(sort $(wildcard $(SRC_DIR)/*.S)) $(sort $(wildcard $(SRC_DIR)/*.cpp))
OBJ		:= $(notdir $(patsubst %.ld,%-$(ARCH).o, $(patsubst %.S,%-$(ARCH).o, $(patsubst %.cpp,%-$(ARCH).o, $(SRC)))))
DEP		:= $(patsubst %.o,%.d, $(OBJ))

# Messages
ifneq ($(findstring s,$(MAKEFLAGS)),)
message = @$(ECHO) $(1) $(2)
endif

# Feature check
check = $(shell if $(CC) $(1) -c -xc++ /dev/null -o /dev/null >/dev/null 2>&1; then echo "$(1)"; fi)

# Preprocessor options
DEFINES		:=
VPATH		:= $(SRC_DIR)
PFLAGS		:= $(addprefix -D, $(DEFINES)) $(addprefix -I, $(INC_DIR))

# Optimization options
DFLAGS		:= -MP -MMD -pipe
OFLAGS		:= -Os
ifeq ($(ARCH),x86_32)
AFLAGS		:= -m32 -march=i686 -mpreferred-stack-boundary=2 -mregparm=3
else ifeq ($(ARCH),x86_64)
AFLAGS		:= -m64 -march=core2 -mpreferred-stack-boundary=4 -mcmodel=kernel -mno-red-zone
else
$(error $(ARCH) is not a valid architecture)
endif

# Language options
FFLAGS		:= -fdata-sections -ffunction-sections -fomit-frame-pointer -freg-struct-return -freorder-blocks -funit-at-a-time
FFLAGS		+= -fno-asynchronous-unwind-tables -fno-exceptions -fno-rtti
FFLAGS		+= $(call check,-fno-stack-protector)
FFLAGS		+= $(call check,-fvisibility-inlines-hidden)
FFLAGS		+= $(or $(call check,-std=gnu++11), $(call check,-std=gnu++0x), $(call check,-std=gnu++98))

# Warning options
WFLAGS		:= -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wformat=2 -Wmissing-format-attribute -Wmissing-noreturn -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wwrite-strings
WFLAGS		+= -Wabi -Wctor-dtor-privacy -Wno-non-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wsign-promo
WFLAGS		+= $(call check,-Wframe-larger-than=64)
WFLAGS		+= $(call check,-Wlogical-op)
WFLAGS		+= $(call check,-Wstrict-null-sentinel)
WFLAGS		+= $(call check,-Wstrict-overflow=5)
WFLAGS		+= $(call check,-Wvolatile-register-var)
WFLAGS		+= $(call check,-Wzero-as-null-pointer-constant)

# Compiler flags
SFLAGS		:= $(PFLAGS) $(DFLAGS) $(AFLAGS)
CFLAGS		:= $(PFLAGS) $(DFLAGS) $(AFLAGS) $(OFLAGS) $(FFLAGS) $(WFLAGS)

# Linker flags
LFLAGS		:= --gc-sections --warn-common -static -n -T

# Rules
%-$(ARCH).o:	%.ld $(MAKEFILE_LIST)
		$(call message,PRE,$@)
		$(CC) $(SFLAGS) -xc -E -P -MT $@ $< -o $@

%-$(ARCH).o:	%.S $(MAKEFILE_LIST)
		$(call message,ASM,$@)
		$(CC) $(SFLAGS) -c $< -o $@

%-$(ARCH).o:	%.cpp $(MAKEFILE_LIST)
		$(call message,CMP,$@)
		$(CC) $(CFLAGS) -c $< -o $@

$(TARGET):	$(OBJ)
		$(call message,LNK,$@)
		$(LD) $(LFLAGS) $^ -o $@

.PHONY:		install
.PHONY:		clean
.PHONY:		cleanall

install:	$(TARGET)
		$(call message,INS,$@)
		$(INSTALL) -s -m 644 $(TARGET) $(INS_DIR)
		@$(SIZE) $<

clean:
		$(call message,CLN,$@)
		$(RM) $(OBJ) $(TARGET)

cleanall:	clean
		$(call message,CLN,$@)
		$(RM) $(DEP)

# Include Dependencies
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanall)
-include	$(DEP)
endif
endif
