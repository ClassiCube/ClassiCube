.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 			= ClassiCube-psp
# List of directories containing source code
SOURCE_DIRS		= src src/psp third_party/bearssl

# Directory where object files are placed
BUILD_DIR		= build/ios
A32_BUILD_DIR	= build/ios/arm7
A64_BUILD_DIR	= build/ios/arm64


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

CFLAGS   := -I$(PSPSDK)/include -g -O1 -fno-math-errno
ASFLAGS  := -I$(PSPSDK)/include -g

LDFLAGS  := -L$(PSPSDK)/lib
LIBS 	 := -lm -lpspgu -lpspge -lpspdisplay -lpspctrl -lpspnet -lpspnet_apctl

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC       = $(PSPDEV)/bin/psp-gcc
AS       = $(PSPDEV)/bin/psp-gcc
LD       = $(PSPDEV)/bin/psp-gcc


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR) $(PSP_EBOOT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(LD) $^ $(LDFLAGS) $(LIBS) -o $@
	$(FIXUP) $@

$(TARGET).prx: $(TARGET).elf
	$(PRXGEN) $< $@


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o : src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : src/ios/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : src/ios/%.S
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o : third_party/bearssl/%.c
	$(CC) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
