BUILD_DIR	:= build-dc
SOURCE_DIRS	:= src third_party/bearssl/src misc/dreamcast
TARGET 		:= ClassiCube-dc

S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))
CFLAGS	:= -g -DNDEBUG -O3 -fipa-pta -fno-pie -flto=auto -fomit-frame-pointer -fbuiltin -ffast-math -ffp-contract=fast -mfsrra -mfsca -pipe -fno-math-errno -Ithird_party/bearssl/inc

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)

GLDC_LIB	= third_party/gldc/libGLdc.a
LDFLAGS		= -g
LIBS		= -lm $(GLDC_LIB) -lppp -lkosfat

ifeq ($(strip $(KOS_BASE)),)
$(warning Please set KOS variables in your environment. For example:)
$(warning source /opt/toolchains/dc/kos/environ.sh)
$(error   Failed to find KallistiOS installation)
endif


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).cdi

clean:
	rm $(TARGET).cdi $(TARGET).iso $(TARGET).elf $(TARGET)-scr.bin $(TARGET).bin $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS) $(GLDC_LIB)
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
# object generation
#---------------------------------------------------------------------------------
$(GLDC_LIB): FORCE
	$(MAKE) -C third_party/gldc
FORCE: ;
	
$(BUILD_DIR)/%.o: src/%.c
	kos-cc $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	kos-cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: misc/dreamcast/%.S
	kos-cc $(DEPFLAGS) -c $< -o $@

# Dependency tracking
$(DEPFILES):

include $(wildcard $(DEPFILES))
