ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

include $(DEVKITPRO)/devkitPPC/base_tools


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 		= ClassiCube-wii
# List of directories containing source code
SOURCE_DIRS	= src src/gcwii third_party/bearssl/src
# List of directories containing more header files
INCLUDES	= -Ithird_party/bearssl/inc
# Directory where object files are placed
BUILD_DIR	= build/wii


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

MACHDEP = -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float
CFLAGS	= -g -O2 -Wall $(MACHDEP) -I$(DEVKITPRO)/libogc/include

LDFLAGS	= -g $(MACHDEP) -L$(DEVKITPRO)/libogc/lib/wii
# Additional libraries to link against
LIBS	= -lasnd -lwiikeyboard -lwiiuse -lbte -lfat -logc -lm

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).dol

clean:
	rm $(TARGET).dol $(TARGET).elf $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)
	
$(TARGET).dol: $(TARGET).elf
	elf2dol $< $@


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/gcwii/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
