ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 		= ClassiCube-switch
# List of directories containing source code
SOURCE_DIRS	= src src/switch third_party/bearssl
# Directory where object files are placed
BUILD_DIR	= build/switch

APP_TITLE 	= ClassiCube
APP_AUTHOR 	= ClassiCube team
APP_ICON    = misc/switch/icon.jpg
APP_VERSION	= 1.3.7


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
ARCH	= -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS	= -g -Wall -O2 -ffunction-sections $(ARCH) -D__SWITCH__ -DPLAT_SWITCH
ASFLAGS	= -g $(ARCH)

LDFLAGS	= -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH)
LIBS	= -lGLESv2 -lEGL -lglapi -ldrm_nouveau -lnx -lm
INCLUDES= 

LIBNX		= $(DEVKITPRO)/libnx
INCLUDES 	+= $(foreach dir, $(LIBNX), -I$(dir)/include)
LDFLAGS 	+= $(foreach dir, $(LIBNX), -L$(dir)/lib)

PORTLIBS	= $(DEVKITPRO)/portlibs/switch
INCLUDES 	+= $(foreach dir, $(PORTLIBS), -I$(dir)/include)
LDFLAGS 	+= $(foreach dir, $(PORTLIBS), -L$(dir)/lib)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-
A64_AS	:= $(PREFIX)as
A64_CC	:= $(PREFIX)gcc
A64_CXX	:= $(PREFIX)g++

ELF2NRO := $(DEVKITPRO)/tools/bin/elf2nro
NACPTOOL := $(DEVKITPRO)/tools/bin/nacptool


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).nro

clean:
	rm $(TARGET).nro $(BUILD_DIR)/$(TARGET).nacp $(TARGET).elf $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(A64_CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/$(TARGET).nacp:
	$(NACPTOOL) --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@ --titleid=$(APP_TITLEID)

$(TARGET).nro: $(TARGET).elf $(BUILD_DIR)/$(TARGET).nacp
	$(ELF2NRO) $< $@ --icon=$(APP_ICON) --nacp=$(BUILD_DIR)/$(TARGET).nacp


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(A64_CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/switch/%.c
	$(A64_CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	$(A64_CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
