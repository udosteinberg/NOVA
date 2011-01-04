#
# Makefile
#
# Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
# Economic rights: Technische Universitaet Dresden (Germany)
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

CC		:= gcc
ECHO		:= echo
INSTALL		:= install
LD		:= ld
RM		:= rm -f
NM		:= nm
SIZE		:= size

SRC_DIR		:= ../src
INC_DIR		:= ../include
INS_DIR		?= /boot/tftp/nova
TARGET		:= hypervisor

SRC		:= hypervisor.ld $(sort $(wildcard $(SRC_DIR)/*.S)) $(sort $(wildcard $(SRC_DIR)/*.cpp))
OBJ		:= $(notdir $(patsubst %.ld,%.o, $(patsubst %.S,%.o, $(patsubst %.cpp,%.o, $(SRC)))))
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
AFLAGS		:= -march=i686 -mpreferred-stack-boundary=2 -mregparm=3
DFLAGS		+= -m32

# Language options
FFLAGS		:= -fdata-sections -ffunction-sections -fomit-frame-pointer -freg-struct-return -freorder-blocks -funit-at-a-time
FFLAGS		+= -fno-exceptions -fno-rtti
FFLAGS		+= $(call check,-fno-stack-protector)
FFLAGS		+= $(call check,-fvisibility-inlines-hidden)

# Warning options
WFLAGS		:= -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wformat=2 -Wmissing-format-attribute -Wmissing-noreturn -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wwrite-strings
WFLAGS		+= -Wabi -Wctor-dtor-privacy -Wno-non-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wsign-promo
WFLAGS		+= $(call check,-Wframe-larger-than=64)
WFLAGS		+= $(call check,-Wlogical-op)
WFLAGS		+= $(call check,-Wstrict-null-sentinel)
WFLAGS		+= $(call check,-Wstrict-overflow=5)
WFLAGS		+= $(call check,-Wvolatile-register-var)

# Compiler flags
SFLAGS		:= $(PFLAGS) $(DFLAGS)
CFLAGS		:= $(PFLAGS) $(DFLAGS) $(OFLAGS) $(AFLAGS) $(FFLAGS) $(WFLAGS)

# Linker flags
LFLAGS		:= --gc-sections --warn-common -static -n -T

# Rules
%.o:		%.ld $(MAKEFILE_LIST)
		$(call message,PRE,$@)
		$(CC) $(SFLAGS) -xc -E -P $< -o $@

%.o:		%.S $(MAKEFILE_LIST)
		$(call message,ASM,$@)
		$(CC) $(SFLAGS) -c $< -o $@

%.o:		%.cpp $(MAKEFILE_LIST)
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
