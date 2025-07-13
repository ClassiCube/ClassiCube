ifeq ($(strip $(PSN00BSDK_ROOT)),)
$(warning Please set PSN00BSDK_ROOT path in your environment. For example:)
$(warning export PSN00BSDK_ROOT=/usr/local/lib/psn00bsdk)
$(error   Failed to find PSN00BSDK installation path)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 		:= ClassiCube-ps1
# List of directories containing source code
SOURCE_DIRS	:= src src/ps1
# Directory where object files are placed
BUILD_DIR	:= build/ps1


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
CINCLUDES := -I$(PSN00BSDK_ROOT)/include/libpsn00b

CFLAGS	:= -DPLAT_PS1 -DPSN00BSDK=1 -g -Wa,--strip-local-absolute -ffreestanding -fno-builtin -nostdlib -fdata-sections -ffunction-sections -fsigned-char -fno-strict-overflow -msoft-float -march=r3000 -mtune=r3000 -mabi=32 -mno-mt -mno-llsc -Og -mdivide-breaks -G8 -fno-pic -mno-abicalls -mgpopt -mno-extern-sdata

# Additional libraries to link against
# libpsxgpu -> $(PSN00BSDK_ROOT)/lib/libpsn00b/debug/libpsxgpu_exe_gprel.a
PS1LIBS	:= libpsxgpu libpsxgte libpsxspu libpsxcd libpsxpress libpsxsio libpsxetc libpsxapi libsmd liblzp libc
PS1LIBS := $(addprefix $(PSN00BSDK_ROOT)/lib/libpsn00b/debug/, $(PS1LIBS))
PS1LIBS := $(addsuffix _exe_gprel.a, $(PS1LIBS))

LDFLAGS		= -g -T$(PSN00BSDK_ROOT)/lib/libpsn00b/ldscripts/exe.ld -nostdlib -Wl,-gc-sections -G8 -static
LIBS		= -lgcc $(PS1LIBS)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).bin

clean:
	rm $(TARGET).bin $(TARGET).cue $(TARGET).exe $(BUILD_DIR)/$(TARGET).elf $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(PSN00BSDK_ROOT)/bin/mipsel-none-elf-gcc $(LDFLAGS) $^ -o $@ $(LIBS)
	
$(TARGET).exe: $(BUILD_DIR)/$(TARGET).elf
	$(PSN00BSDK_ROOT)/bin/elf2x -q $(BUILD_DIR)/$(TARGET).elf $(TARGET).exe
	
$(TARGET).bin: $(TARGET).exe
	$(PSN00BSDK_ROOT)/bin/mkpsxiso -y -o $(TARGET).bin -c $(TARGET).cue misc/ps1/iso.xml


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(PSN00BSDK_ROOT)/bin/mipsel-none-elf-gcc $(CFLAGS) $(CINCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/ps1/%.c
	$(PSN00BSDK_ROOT)/bin/mipsel-none-elf-gcc $(CFLAGS) $(CINCLUDES) $(DEPFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
