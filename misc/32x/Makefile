ifdef $(GENDEV)
ROOTDIR = $(GENDEV)
else
ROOTDIR = /opt/toolchains/sega
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-32x
SOURCE_DIRS := src src/32x misc/32x
BUILD_DIR 	:= build/32x
RUN_PROGRAM	?= ares

default: $(TARGET).bin


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
LIBS := $(LIBPATH) -lc -lgcc -lgcc-Os-4-200 -lnosys

CFLAGS 	:= -m2 -mb -Ofast -Wall -g -c -fomit-frame-pointer -DPLAT_32X -ffunction-sections -fdata-sections \
			-I$(ROOTDIR)/sh-elf/include -I$(ROOTDIR)/sh-elf/sh-elf/include
LDFLAGS := -T $(ROOTDIR)/ldscripts/mars.ld \
			-nostdlib -Wl,--gc-sections \
			-L$(ROOTDIR)/sh-elf/lib -L$(ROOTDIR)/sh-elf/lib/gcc/sh-elf/4.6.2 -L$(ROOTDIR)/sh-elf/sh-elf/lib
ASFLAGS := --big

MDLDFLAGS := -T $(ROOTDIR)/ldscripts/md.ld \
			--oformat binary
MDASFLAGS := -m68000 --register-prefix-optional


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
SHPREFIX := $(ROOTDIR)/sh-elf/bin/sh-elf-
CC 		 := $(SHPREFIX)gcc
AS 		 := $(SHPREFIX)as

OBJCOPY  := $(SHPREFIX)objcopy
LINK	 := $(CC)

MDPREFIX := $(ROOTDIR)/m68k-elf/bin/m68k-elf-
MDAS 	 := $(MDPREFIX)as
MDLD 	 := $(MDPREFIX)ld


#---------------------------------------------------------------------------------
# CRT generation
#---------------------------------------------------------------------------------
MD_DIR	:= misc/32x
MD_SRCS := $(notdir $(wildcard $(MD_DIR)/m68k*.s))

MD_OBJS := $(addprefix $(BUILD_DIR)/, $(MD_SRCS:.s=.o))
MD_BINS := $(MD_OBJS:.o=.bin)

SH_CRT		:= $(BUILD_DIR)/sh2_crt0.o
OBJECTS 	+= $(SH_CRT)
GEN_FILES	+= $(MD_OBJS) $(MD_BINS)

$(SH_CRT): misc/32x/sh2_crt0.s $(MD_BINS)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/m68k_crt%.bin: $(BUILD_DIR)/m68k_crt%.o
	$(MDLD) $(MDLDFLAGS) $< -o $@

$(BUILD_DIR)/m68k_crt%.o: $(MD_DIR)/m68k_crt%.s
	$(MDAS) $(MDASFLAGS) $< -o $@


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).bin $(BUILD_DIR)/temp.bin
include misc/makefiles/common_build.mk

$(BUILD_DIR)/temp.bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).bin: $(BUILD_DIR)/temp.bin
	dd if=$< of=$@ bs=64K conv=sync


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
