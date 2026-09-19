ifeq ($(strip $(PLAYDATE_SDK_PATH)),)
$(error "Please set PLAYDATE_SDK_PATH in your environment. export PLAYDATE_SDK_PATH=<path to>PlaydateSDK)
endif
BUILD_SIMULATOR ?= 1
.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-playdate
SOURCE_DIRS	:= src src/playdate

GAME_TITLE	:= ClassiCube
GAME_CODE	:= 0000
MAKER_CODE	:= 00

default: $(TARGET).bin

LDSCRIPT = $(patsubst ~%,$(HOME)%,$(PLAYDATE_SDK_PATH)/C_API/buildsupport/link_map.ld)


#---------------------------------------------------------------------------------
# Code generation options
#---------------------------------------------------------------------------------
ifeq ($(BUILD_SIMULATOR),1)
BUILD_DIR	:= build/playdate_sim
CFLAGS := -fPIC -lm -DTARGET_SIMULATOR=1 -DTARGET_EXTENSION=1 -DPLAT_PLAYDATE -I$(PLAYDATE_SDK_PATH)/C_API
LDFLAGS := -shared
OEXT		:= .so
PREFIX	:=
else
BUILD_DIR	:= build/playdate
MAP		:= $(BUILD_DIR)/$(NAME).map
ARCH	:= -mthumb -mcpu=cortex-m7 -mtune=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -D__FPU_USED=1
CFLAGS	:= -gdwarf-2 -Wall -Wno-unused -Wstrict-prototypes -Wno-unknown-pragmas -fverbose-asm -Wdouble-promotion -mword-relocations -fno-common -Wstack-usage=8192 -Walloca-larger-than=8192 -O2 -I$(PLAYDATE_SDK_PATH)/C_API -DTARGET_PLAYDATE=1 -DTARGET_EXTENSION=1 -DPLAT_PLAYDATE -ffunction-sections -fdata-sections $(ARCH)
LDFLAGS	:= -nostartfiles $(ARCH) -T$(LDSCRIPT) -Wl,-Map=$(MAP),--cref,--gc-sections,--no-warn-mismatch,--emit-relocs
OEXT		:= .elf
PREFIX	:= arm-none-eabi-
endif

#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------

CC		:= $(PREFIX)gcc
LINK	:= $(CC)

OBJCOPY := $(PREFIX)objcopy


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------

GEN_FILES 	:= $(TARGET)$(OEXT)
include misc/makefiles/common_build.mk


$(TARGET).bin: $(TARGET)$(OEXT)
	cp $(TARGET)$(OEXT) misc/playdate/Classicube/pdex$(OEXT)
	$(PLAYDATE_SDK_PATH)/bin/pdc -sdkpath $(PLAYDATE_SDK_PATH) misc/playdate/Classicube
	cp -r misc/playdate/Classicube.pdx ./
	


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
