ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-3ds
SOURCE_DIRS	:= src src/3ds third_party/bearssl
BUILD_DIR	:= build/3ds
RUN_PROGRAM	?= azahar

APP_ICON	:= misc/3ds/icon.png
APP_TITLE 	:= ClassiCube
APP_DESC	:= Simple block building sandbox
APP_AUTHOR 	:= ClassiCube team

CIA_BANNER_BIN	:= misc/3ds/banner.bin
CIA_ICON_BIN	:= misc/3ds/icon.bin
CIA_SPEC_RSF	:= misc/3ds/spec.rsf

SMDH_FILE	:= $(BUILD_DIR)/$(TARGET).smdh

default: $(TARGET).cia $(TARGET).3dsx


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
ARCH	:= -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS	:= -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) \
			-I$(DEVKITPRO)/libctru/include \
			-D__3DS__ -DPLAT_3DS

LDFLAGS	:= -specs=3dsx.specs -g $(ARCH) \
			-L$(DEVKITPRO)/libctru/lib
LIBS	:= -lctru -lm


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitARM/bin/arm-none-eabi-
CC		:= $(PREFIX)gcc
LINK	:= $(CC)

TDSXTOOL := $(DEVKITPRO)/tools/bin/3dsxtool
SMDHTOOL := $(DEVKITPRO)/tools/bin/smdhtool
PICASSO	 := $(DEVKITPRO)/tools/bin/picasso


#---------------------------------------------------------------------------------
# Shader generation
#---------------------------------------------------------------------------------
SHDR_DIR	:= misc/3ds
SHDR_SRCS 	:= $(notdir $(wildcard $(SHDR_DIR)/*.v.pica))
SHDR_OBJS	:= $(addprefix $(BUILD_DIR)/, $(SHDR_SRCS:.v.pica=.shbin))
GEN_FILES	+= $(SHDR_OBJS)

$(BUILD_DIR)/%.shbin: $(SHDR_DIR)/%.v.pica
	$(PICASSO) $< -o $@

src/3ds/Graphics_3DS.c: $(SHDR_OBJS)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).cia $(TARGET).3dsx $(SMDH_FILE)
include misc/makefiles/common_build.mk

$(BUILD_DIR)/makerom:
	wget https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.18.3/makerom-v0.18.3-ubuntu_x86_64.zip -O $(BUILD_DIR)/makerom.zip
	unzip $(BUILD_DIR)/makerom.zip -d $(BUILD_DIR)
	chmod +x $(BUILD_DIR)/makerom

$(SMDH_FILE): $(APP_ICON)
	$(SMDHTOOL) --create "$(APP_TITLE)" "$(APP_DESC)" "$(APP_AUTHOR)" $(APP_ICON) $@

$(TARGET).3dsx: $(TARGET).elf $(SMDH_FILE)
	$(TDSXTOOL) $< $@ --smdh=$(SMDH_FILE)

$(TARGET).cia: $(TARGET).elf $(BUILD_DIR)/makerom $(CIA_BANNER_BIN) $(CIA_ICON_BIN) $(CIA_SPEC_RSF)
	$(BUILD_DIR)/makerom -f cia -o $@ -elf $< -rsf $(CIA_SPEC_RSF) -icon $(CIA_ICON_BIN) -banner $(CIA_BANNER_BIN) -exefslogo -target t


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
