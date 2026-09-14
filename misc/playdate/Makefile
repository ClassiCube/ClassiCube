ifeq ($(strip $(PLAYDATE_SDK_PATH)),)
$(error "Please set PLAYDATE_SDK_PATH in your environment. export PLAYDATE_SDK_PATH=<path to>PlaydateSDK)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-playdate
SOURCE_DIRS	:= src src/playdate
BUILD_DIR	:= build/playdate

GAME_TITLE	:= ClassiCube
GAME_CODE	:= 0000
MAKER_CODE	:= 00

default: $(TARGET).bin

LDSCRIPT = $(patsubst ~%,$(HOME)%,$(PLAYDATE_SDK_PATH)/C_API/buildsupport/link_map.ld)


#---------------------------------------------------------------------------------
# Code generation options
#---------------------------------------------------------------------------------
MAP		:= $(BUILD_DIR)/$(NAME).map
ARCH	:= -mthumb -mcpu=cortex-m7 -mtune=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -D__FPU_USED=1
CFLAGS	:= -gdwarf-2 -Wall -Wno-unused -Wstrict-prototypes -Wno-unknown-pragmas -fverbose-asm -Wdouble-promotion -mword-relocations -fno-common -Wstack-usage=8192 -Walloca-larger-than=8192 -O2 -I$(PLAYDATE_SDK_PATH)/C_API -DTARGET_PLAYDATE=1 -DTARGET_EXTENSION=1 -DPLAT_PLAYDATE -ffunction-sections -fdata-sections $(ARCH)
#CFLAGS := -fPIC -lm -DTARGET_SIMULATOR=1 -DTARGET_EXTENSION=1 -DPLAT_PLAYDATE -I$(PLAYDATE_SDK_PATH)/C_API
LDFLAGS	:= -nostartfiles $(ARCH) -T$(LDSCRIPT) -Wl,-Map=$(MAP),--cref,--gc-sections,--no-warn-mismatch,--emit-relocs
#LDFLAGS := -shared 


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= arm-none-eabi-
CC		:= $(PREFIX)gcc
LINK	:= $(CC)

OBJCOPY := $(PREFIX)objcopy


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).elf
include misc/makefiles/common_build.mk


$(TARGET).bin: $(TARGET).elf
	cp $(TARGET).elf misc/playdate/Classicube/pdex.elf
	$(PLAYDATE_SDK_PATH)/bin/pdc -sdkpath $(PLAYDATE_SDK_PATH) misc/playdate/Classicube
	cp -r misc/playdate/Classicube.pdx ./
	


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
