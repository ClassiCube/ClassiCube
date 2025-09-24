ifeq ($(strip $(KOS_BASE)),)
$(warning Please set KOS variables in your environment. For example:)
$(warning source /opt/toolchains/dc/kos/environ.sh)
$(error   Failed to find KallistiOS installation)
endif


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Directory where object files are placed
BUILD_DIR	:= build/dc
# List of directories containing source code
SOURCE_DIRS	:= src third_party/bearssl src/dreamcast
# Name of the final output
TARGET 		:= ClassiCube-dc


#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS	= -g -DNDEBUG -O3 -fipa-pta -fno-pie -flto=auto -fomit-frame-pointer -fbuiltin -ffast-math -ffp-contract=fast -mfsrra -mfsca -pipe -fno-math-errno -DPLAT_DREAMCAST
LDFLAGS	= -g

# Additional libraries to link against
LIBS		= -lm -lppp -lkosfat

#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).cdi

clean:
	rm $(TARGET).cdi $(TARGET).iso $(TARGET).elf $(TARGET)-scr.bin $(TARGET).bin $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	kos-cc $(LDFLAGS) $^ -o $@ $(LIBS)
	
$(TARGET).bin: $(TARGET).elf
	sh-elf-objcopy -R .stack -O binary $(TARGET).elf $(TARGET).bin
	
# https://dcemulation.org/phpBB/viewtopic.php?t=105269
$(TARGET)-scr.bin: $(TARGET).bin
	$(KOS_BASE)/utils/scramble/scramble $(TARGET).bin $(TARGET)-scr.bin
	
$(TARGET).iso: $(TARGET)-scr.bin
	mkdir -p ISO_FILES
	cp $(TARGET)-scr.bin ISO_FILES/1ST_READ.BIN
	mkdir -p ISO_FILES/audio
	mkdir -p ISO_FILES/maps
	mkdir -p ISO_FILES/texpacks
	mkdir -p ISO_FILES/texturecache
	cp misc/dreamcast/classicube.zip ISO_FILES/texpacks/default.zip
	cp misc/dreamcast/IP.BIN IP.BIN
	mkisofs -G IP.BIN -C 0,11702 -J -l -r -quiet -o $(TARGET).iso ISO_FILES
# genisoimage -V ClassiCube -G IP.BIN -joliet -rock -l -o $(TARGET).iso ISO_FILES
	
$(TARGET).cdi: $(TARGET).iso
	cdi4dc $(TARGET).iso $(TARGET).cdi


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	kos-cc $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/dreamcast/%.c
	kos-cc $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	kos-cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/dreamcast/%.S
	kos-cc $(DEPFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
