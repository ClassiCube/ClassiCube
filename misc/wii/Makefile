ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Directory where object files are placed
BUILD_DIR	:= build/wii
# List of directories containing source code
SOURCE_DIRS	:= src third_party/bearssl/src
# Name of the final output
TARGET 		:= ClassiCube-wii
# Additional libraries to link against
LIBS		= -lasnd -lwiikeyboard -lwiiuse -lbte -lfat -logc -lm
# List of directories containing more header files
INCLUDES	= -Ithird_party/bearssl/inc


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))
CFLAGS	= -g -O2 -Wall $(MACHDEP) -I$(LIBOGC_INC)
LDFLAGS	= -g $(MACHDEP) -L$(LIBOGC_LIB)

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

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
