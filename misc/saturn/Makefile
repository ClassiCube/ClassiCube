ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
$(warning Please set YAUL variables in your environment. For example:)
$(warning source libyaul/yaul.env.in)
$(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-saturn
SOURCE_DIRS := src src/saturn
BUILD_DIR	:= build/saturn
RUN_PROGRAM	?= mednafen

IP_VERSION				= V1.370
IP_RELEASE_DATE			= 20250101
IP_AREAS				= JTUBKAEL
IP_PERIPHERALS			= JAMKST
IP_TITLE				= ClassiCube
#IP_MASTER_STACK_ADDR	= 0x06004000
IP_MASTER_STACK_ADDR	= 0x06100000
IP_SLAVE_STACK_ADDR		= 0x06001E00
IP_1ST_READ_ADDR		= 0x06004000
IP_1ST_READ_SIZE		= 0

CD_PATH					= $(BUILD_DIR)/cd
AUDIO_TRACKS_DIRECTORY	= audio-tracks
IMAGE_1ST_READ_BIN		= A.BIN


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
SPECS	:= -specs=yaul.specs -specs=yaul-main.specs
CFLAGS 	:= -Os -DPLAT_SATURN -Wstrict-aliasing $(SPECS) \
	-I$(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include/yaul

LDFLAGS := -static -Wl,--gc-sections $(SPECS) \
	-Wl,--defsym=___master_stack=$(IP_MASTER_STACK_ADDR) \
	-Wl,--defsym=___slave_stack=$(IP_SLAVE_STACK_ADDR)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC		:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-gcc
OBJCOPY	:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-objcopy
LINK	:= $(CC)

M68K_AS:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-as
M68K_CC:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-gcc
M68K_LD:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-gcc
M68K_OBJCOPY:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-objcopy

MAKE_CUE	:= $(YAUL_INSTALL_ROOT)/bin/make-cue
MAKE_ISO	:= $(YAUL_INSTALL_ROOT)/bin/make-iso
MAKE_IP		:= $(YAUL_INSTALL_ROOT)/bin/make-ip


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).cue: $(TARGET).iso
	mkdir -p $(AUDIO_TRACKS_DIRECTORY)
	$(MAKE_CUE) $(AUDIO_TRACKS_DIRECTORY) $<

$(TARGET).iso: $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/IP.BIN
	mkdir -p $(CD_PATH)
	cp $< $(CD_PATH)/$(IMAGE_1ST_READ_BIN)
	echo "empty" > $(CD_PATH)/ABS.TXT
	echo "empty" > $(CD_PATH)/BIB.TXT
	echo "empty" > $(CD_PATH)/CPY.TXT
	$(MAKE_ISO) $(CD_PATH) $(BUILD_DIR)/IP.BIN . $(TARGET)

$(BUILD_DIR)/IP.BIN: $(BUILD_DIR)/$(TARGET).bin
	$(MAKE_IP) $< \
		$(IP_VERSION) $(IP_RELEASE_DATE) $(IP_AREAS) $(IP_PERIPHERALS) \
		'"$(IP_TITLE)"' \
		$(IP_MASTER_STACK_ADDR) $(IP_SLAVE_STACK_ADDR) \
		$(IP_1ST_READ_ADDR) $(IP_1ST_READ_SIZE)

$(BUILD_DIR)/$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@


OEXT		:= .elf
GEN_FILES 	:= $(TARGET).cue $(TARGET).iso $(BUILD_DIR)/IP.BIN $(BUILD_DIR)/$(TARGET).bin
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
