ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Directory where object files are placed
BUILD_DIR	= build/saturn
# List of directories containing source code
SOURCE_DIRS = src src/saturn
# Name of the final output
TARGET 		= ClassiCube-saturn

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
# Compilable files
#---------------------------------------------------------------------------------
S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
SH_OBJS := $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(SH_OBJS:%.o=%.d)



#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
SH_CFLAGS = -Os -DPLAT_SATURN -Wstrict-aliasing \
	-I$(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include/yaul

SH_LDFLAGS = -static -Wl,--gc-sections \
	-Wl,--defsym=___master_stack=$(IP_MASTER_STACK_ADDR) \
	-Wl,--defsym=___slave_stack=$(IP_SLAVE_STACK_ADDR)

SH_SPECS = yaul.specs yaul-main.specs


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
SH_AS:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-as
SH_CC:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-gcc
SH_LD:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-gcc
SH_OBJCOPY:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-objcopy

M68K_AS:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-as
M68K_CC:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-gcc
M68K_LD:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-gcc
M68K_OBJCOPY:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_M68K_PREFIX)-objcopy


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).cue

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(SH_OBJS) $(TARGET).bin $(TARGET).cue $(TARGET).iso \
	    $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/IP.BIN 


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/$(TARGET).elf: $(SH_OBJS)
	$(SH_LD) $(foreach specs,$(SH_SPECS),-specs=$(specs)) $(SH_OBJS) $(SH_LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(SH_OBJCOPY) -O binary $< $@
	@[ -z "${SILENT}" ] && du -hs $@ | awk '{ print $$1; }' || true

$(TARGET).bin: $(BUILD_DIR)/$(TARGET).bin
	cp $< $@

$(BUILD_DIR)/IP.BIN: $(TARGET).bin
	$(YAUL_INSTALL_ROOT)/bin/make-ip \
	    "$(BUILD_DIR)/$(TARGET).bin" \
		$(IP_VERSION) $(IP_RELEASE_DATE) $(IP_AREAS) $(IP_PERIPHERALS) \
		'"$(IP_TITLE)"' \
		$(IP_MASTER_STACK_ADDR) $(IP_SLAVE_STACK_ADDR) \
		$(IP_1ST_READ_ADDR) $(IP_1ST_READ_SIZE)

$(TARGET).iso: $(TARGET).bin $(BUILD_DIR)/IP.BIN
	mkdir -p $(CD_PATH)
	cp $(TARGET).bin $(CD_PATH)/$(IMAGE_1ST_READ_BIN)
	printf "empty\n" > $(CD_PATH)/"ABS.TXT"
	printf "empty\n" > $(CD_PATH)/"BIB.TXT"
	printf "empty\n" > $(CD_PATH)/"CPY.TXT"
	$(YAUL_INSTALL_ROOT)/bin/make-iso $(CD_PATH) $(BUILD_DIR)/IP.BIN . $(TARGET)

$(TARGET).cue: $(TARGET).iso
	mkdir -p $(AUDIO_TRACKS_DIRECTORY)
	$(YAUL_INSTALL_ROOT)/bin/make-cue $(AUDIO_TRACKS_DIRECTORY) $(TARGET).iso


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o : src/%.c
	$(SH_CC) $(SH_CFLAGS) $(DEPFLAGS) $(foreach specs,$(SH_SPECS),-specs=$(specs)) -c $< -o $@

$(BUILD_DIR)/%.o : src/saturn/%.c
	$(SH_CC) $(SH_CFLAGS) $(DEPFLAGS) $(foreach specs,$(SH_SPECS),-specs=$(specs)) -c $< -o $@

$(BUILD_DIR)/%.o : src/saturn/%.S
	$(SH_CC) $(SH_CFLAGS) $(DEPFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
