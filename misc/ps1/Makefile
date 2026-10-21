ifeq ($(strip $(PSN00BSDK_ROOT)),)
$(warning Please set PSN00BSDK_ROOT path in your environment. For example:)
$(warning export PSN00BSDK_ROOT=/usr/local/lib/psn00bsdk)
$(error   Failed to find PSN00BSDK installation path)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-ps1
SOURCE_DIRS	:= src src/ps1
BUILD_DIR	:= build/ps1
RUN_PROGRAM	?= duckstation

default: $(TARGET).bin


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS	:= -g -Wa,--strip-local-absolute -ffreestanding -fno-builtin -nostdlib -fdata-sections -ffunction-sections -fno-strict-overflow -msoft-float -march=r3000 -mtune=r3000 -mabi=32 -mno-mt -mno-llsc -Og -mdivide-breaks -G8 -fno-pic -mno-abicalls -mgpopt -mno-extern-sdata \
	-DPLAT_PS1 -DPSN00BSDK=1 -Wstack-usage=50000 \
	-I$(PSN00BSDK_ROOT)/include/libpsn00b

# Additional libraries to link against
# libpsxgpu -> $(PSN00BSDK_ROOT)/lib/libpsn00b/debug/libpsxgpu_exe_gprel.a
PS1LIBS	:= libpsxgpu libpsxgte libpsxspu libpsxcd libpsxpress libpsxsio libpsxetc libpsxapi libsmd liblzp libc
PS1LIBS := $(addprefix $(PSN00BSDK_ROOT)/lib/libpsn00b/debug/, $(PS1LIBS))
PS1LIBS := $(addsuffix _exe_gprel.a, $(PS1LIBS))

LDFLAGS	:= -g -T$(PSN00BSDK_ROOT)/lib/libpsn00b/ldscripts/exe.ld -nostdlib -Wl,-gc-sections -G8 -static
LIBS	:= -lgcc $(PS1LIBS)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(PSN00BSDK_ROOT)/bin/mipsel-none-elf-
CC		:= $(PREFIX)gcc
LINK	:= $(CC)

MKPSXISO 	:= $(PSN00BSDK_ROOT)/bin/mkpsxiso
ELF2X		:= $(PSN00BSDK_ROOT)/bin/elf2x


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).exe $(TARGET).bin $(TARGET).cue
include misc/makefiles/common_build.mk
	
$(TARGET).exe: $(TARGET).elf
	$(PSN00BSDK_ROOT)/bin/elf2x -q $(TARGET).elf $(TARGET).exe

$(TARGET).bin: $(TARGET).exe
	$(PSN00BSDK_ROOT)/bin/mkpsxiso -y -o $(TARGET).bin -c $(TARGET).cue misc/ps1/iso.xml


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
