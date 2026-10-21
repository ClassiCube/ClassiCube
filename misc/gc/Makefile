ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-gc
SOURCE_DIRS	:= src src/gcwii third_party/bearssl
BUILD_DIR	:= build/gc
RUN_PROGRAM	?= dolphin

default: $(TARGET).dol


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
MACHDEP := -DGEKKO -mogc -mcpu=750 -meabi -msdata=sysv -mhard-float
CFLAGS	:= -g -O2 -Wall $(MACHDEP) -I$(DEVKITPRO)/libogc2/gamecube/include -DPLAT_GAMECUBE

LDFLAGS	:= -g $(MACHDEP) -L$(DEVKITPRO)/libogc2/gamecube/lib
LIBS	:= -lasnd -lbba -lfat -logc -lm


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitPPC/bin/powerpc-eabi-
CC		:= $(PREFIX)gcc
LINK	:= $(CC)

ELF2DOL := $(DEVKITPRO)/tools/bin/elf2dol


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).dol: $(TARGET).elf
	$(ELF2DOL) $< $@

OEXT		:= .elf
GEN_FILES 	:= $(TARGET).dol
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
