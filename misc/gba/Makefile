ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-gba
SOURCE_DIRS	:= src src/gba
BUILD_DIR	:= build/gba
RUN_PROGRAM	?= mgba

GAME_TITLE	:= ClassiCube
GAME_CODE	:= 0000
MAKER_CODE	:= 00

default: $(TARGET).gba


#---------------------------------------------------------------------------------
# Code generation options
#---------------------------------------------------------------------------------
ARCH	:= -mthumb -mthumb-interwork
CFLAGS	:= -g -gdwarf-4 -Wall -O2 -DPLAT_GBA -ffunction-sections -fdata-sections -mcpu=arm7tdmi -mtune=arm7tdmi $(ARCH)
LDFLAGS	:= -specs=gba.specs -g $(ARCH)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitARM/bin/arm-none-eabi-
CC		:= $(PREFIX)gcc
LINK	:= $(CC)

OBJCOPY := $(PREFIX)objcopy
GBAFIX 	:= $(DEVKITPRO)/tools/bin/gbafix


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).gba
include misc/makefiles/common_build.mk

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -t$(GAME_TITLE) -c$(GAME_CODE) -m$(MAKER_CODE)


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
