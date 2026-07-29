ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-switch
SOURCE_DIRS	:= src src/switch third_party/bearssl
BUILD_DIR	:= build/switch
RUN_PROGRAM	?= ryujinx

APP_TITLE 	:= ClassiCube
APP_AUTHOR 	:= ClassiCube team
APP_ICON    := misc/switch/icon.jpg
APP_VERSION	:= 1.3.8


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
ARCH	= -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS	= -g -Wall -O2 -ffunction-sections $(ARCH) -D__SWITCH__ -DPLAT_SWITCH

LDFLAGS	= -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH)
LIBS	= -lGLESv2 -lEGL -lglapi -ldrm_nouveau -lnx -lm
INCLUDES= 

LIBNX		= $(DEVKITPRO)/libnx
INCLUDES 	+= -I$(LIBNX)/include
LDFLAGS 	+= -L$(LIBNX)/lib

PORTLIBS	= $(DEVKITPRO)/portlibs/switch
INCLUDES 	+= -I$(PORTLIBS)/include
LDFLAGS 	+= -L$(PORTLIBS)/lib

CFLAGS += $(INCLUDES)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-
CC		:= $(PREFIX)gcc
LINK	:= $(PREFIX)g++

ELF2NRO  := $(DEVKITPRO)/tools/bin/elf2nro
NACPTOOL := $(DEVKITPRO)/tools/bin/nacptool


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).nro: $(TARGET).elf $(BUILD_DIR)/$(TARGET).nacp
	$(ELF2NRO) $< $@ --icon=$(APP_ICON) --nacp=$(BUILD_DIR)/$(TARGET).nacp

$(BUILD_DIR)/$(TARGET).nacp:
	$(NACPTOOL) --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@ --titleid=$(APP_TITLEID)


OEXT		:= .elf
GEN_FILES 	:= $(TARGET).nro $(BUILD_DIR)/$(TARGET).nacp
include misc/makefiles/common_build.mk
