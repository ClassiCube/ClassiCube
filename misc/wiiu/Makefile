ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-wiiu
SOURCE_DIRS	:= src src/wiiu third_party/bearssl
BUILD_DIR	:= build/wiiu
RUN_PROGRAM	?= cemu

WUHB_OPTIONS := \
	--name "ClassiCube" \
	--short-name "ClassiCube" \
	--author "ClassiCube team" \
	--icon=misc/wiiu/icon.png

#WUHB_OPTIONS += --tv-image=misc/wiiu/tv_splash.png
#WUHB_OPTIONS += --drc-image=misc/wiiu/drc_splash.png


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
MACHDEP	= -DESPRESSO -mcpu=750 -meabi -mhard-float
CFLAGS	= -g -Wall -O2 -ffunction-sections -fno-math-errno $(MACHDEP) \
			-I $(DEVKITPRO)/wut/include \
			-D__WIIU__ -D__WUT__ -DPLAT_WIIU

LDFLAGS	= -g $(MACHDEP) -specs=$(DEVKITPRO)/wut/share/wut.specs \
			-L $(DEVKITPRO)/wut/lib
LIBS	= -lwut -lm


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitPPC/bin/powerpc-eabi-
CC		:= $(PREFIX)gcc
LINK	:= $(CC)

ELF2RPL	 := $(DEVKITPRO)/tools/bin/elf2rpl
WUHBTOOL := $(DEVKITPRO)/tools/bin/wuhbtool


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).wuhb: $(TARGET).rpx
	$(WUHBTOOL) $< $@ $(WUHB_OPTIONS)
	
$(TARGET).rpx: $(TARGET).elf
	$(ELF2RPL) $< $@


OEXT		:= .elf
GEN_FILES 	:= $(TARGET).wuhb $(TARGET).rpx
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# Shader generation
#---------------------------------------------------------------------------------
SHDR_DIR	:= misc/wiiu
SHDR_OBJS 	:= $(wildcard $(SHDR_DIR)/*.gsh)

src/wiiu/Graphics_WiiU.c: $(SHDR_OBJS)
