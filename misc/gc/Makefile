ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 		= ClassiCube-gc
# List of directories containing source code
SOURCE_DIRS	= src src/gcwii third_party/bearssl
# Directory where object files are placed
BUILD_DIR	= build/gc


#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------

MACHDEP = -DGEKKO -mogc -mcpu=750 -meabi -msdata=sysv -mhard-float
CFLAGS	= -g -O2 -Wall $(MACHDEP) -I$(DEVKITPRO)/libogc2/gamecube/include -DPLAT_GAMECUBE

LDFLAGS	= -g $(MACHDEP) -L$(DEVKITPRO)/libogc2/gamecube/lib
# Additional libraries to link against
LIBS	= -lasnd -lbba -lfat -logc -lm


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitPPC/bin/powerpc-eabi-
PPC_AS	:= $(PREFIX)as
PPC_CC	:= $(PREFIX)gcc
PPC_CXX	:= $(PREFIX)g++

ELF2DOL := $(DEVKITPRO)/tools/bin/elf2dol


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
	$(PPC_CC) $(LDFLAGS) $^ -o $@ $(LIBS)
	
$(TARGET).dol: $(TARGET).elf
	$(ELF2DOL) $< $@


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(PPC_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/gcwii/%.c
	$(PPC_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	$(PPC_CC) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
