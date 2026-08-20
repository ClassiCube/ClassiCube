ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-wii
SOURCE_DIRS	:= src src/gcwii third_party/bearssl
BUILD_DIR	:= build/wii
RUN_PROGRAM	?= dolphin

default: $(TARGET).dol


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
MACHDEP := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float
CFLAGS	:= -g -O2 -Wall $(MACHDEP) -I$(DEVKITPRO)/libogc/include -DPLAT_WII

LDFLAGS	:= -g $(MACHDEP) -L$(DEVKITPRO)/libogc/lib/wii
LIBS	:= -lasnd -lwiikeyboard -lwiiuse -lbte -lfat -logc -lm


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
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).dol
include misc/makefiles/common_build.mk

$(TARGET).dol: $(TARGET).elf
	$(ELF2DOL) $< $@


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
