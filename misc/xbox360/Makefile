ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON in your environment. export DEVKITXENON=<path to>devkitPPC")
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET		:=	ClassiCube-xbox360
# Directory where object files are placed
BUILD_DIR	:=	build/xbox360
# List of directories containing source code
SOURCE_DIRS	:=	src src/xbox360 third_party/bearssl


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
MACHDEP = -DXENON -m32 -maltivec -fno-pic -mpowerpc64 -mhard-float -L$(DEVKITXENON)/xenon/lib/32
LDSCRIPT = $(DEVKITXENON)/app.lds

CFLAGS	= -g -O2 -Wall $(MACHDEP) $(INCLUDE)
LDFLAGS	= -g $(MACHDEP) -Wl,--gc-sections
LIBS	= -lxenon -lm -lfat

CFLAGS  += -I$(DEVKITXENON)/usr/include
LDFLAGS += -L$(DEVKITXENON)/usr/lib


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).elf32

clean:
	rm -f $(TARGET).elf $(TARGET).elf32 $(OBJS) $(DEPFILES)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	xenon-gcc $^ $(LDFLAGS) $(LIBS) -n -T $(LDSCRIPT) -o $@

$(TARGET).elf32: $(TARGET).elf
	xenon-objcopy -O elf32-powerpc --adjust-vma 0x80000000 $< $@
	xenon-strip $@


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	xenon-gcc $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/xbox360/%.c
	xenon-gcc $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/xbox360/%.S
	xenon-gcc $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	xenon-gcc $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
