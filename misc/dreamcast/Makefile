ifeq ($(strip $(KOS_BASE)),)
$(warning Please set KOS variables in your environment. For example:)
$(warning source /opt/toolchains/dc/kos/environ.sh)
$(error   Failed to find KallistiOS installation)
endif


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
BUILD_DIR	:= build/dc
SOURCE_DIRS	:= src third_party/bearssl src/dreamcast
TARGET 		:= ClassiCube-dc
RUN_PROGRAM	?= flycast

ISO_DIR		:= $(BUILD_DIR)/ISO_FILES


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS	:= -g -DNDEBUG -O3 -fipa-pta -fno-pie -flto=auto -fomit-frame-pointer -fbuiltin -ffast-math -ffp-contract=fast -mfsrra -mfsca -pipe -fno-math-errno -DPLAT_DREAMCAST
LDFLAGS	:= -g
LIBS	:= -lm -lppp -lkosfat


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC		:= kos-cc
OBJCOPY := sh-elf-objcopy
LINK	:= $(CC)

CDI4DC		:= cdi4dc
SCRAMBLE 	:= $(KOS_BASE)/utils/scramble/scramble


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).cdi: $(TARGET).iso
	$(CDI4DC) $< $@
	
$(TARGET).iso: $(TARGET)-scr.bin
	mkdir -p $(ISO_DIR)
	cp $< $(ISO_DIR)/1ST_READ.BIN
	mkdir -p $(ISO_DIR)/audio
	mkdir -p $(ISO_DIR)/maps
	mkdir -p $(ISO_DIR)/texpacks
	mkdir -p $(ISO_DIR)/texturecache
	cp misc/dreamcast/classicube.zip $(ISO_DIR)/texpacks/default.zip
	cp misc/dreamcast/IP.BIN IP.BIN
	mkisofs -G IP.BIN -C 0,11702 -J -l -r -quiet -o $@ $(ISO_DIR)
# genisoimage -V ClassiCube -G IP.BIN -joliet -rock -l -o $@ $(ISO_DIR)

# https://dcemulation.org/phpBB/viewtopic.php?t=105269
$(TARGET)-scr.bin: $(TARGET).bin
	$(SCRAMBLE) $< $@
	
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -R .stack -O binary $< $@


OEXT		:= .elf
GEN_FILES 	:= $(TARGET).cdi $(TARGET).iso $(TARGET)-scr.bin $(TARGET).bin
include misc/makefiles/common_build.mk
