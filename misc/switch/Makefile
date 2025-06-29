ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

include $(DEVKITPRO)/devkitA64/base_tools


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 		= ClassiCube-switch
# List of directories containing source code
SOURCE_DIRS	= src src/switch third_party/bearssl/src
# List of directories containing more header files
INCLUDES	= -Ithird_party/bearssl/inc
# Directory where object files are placed
BUILD_DIR	= build/switch

APP_TITLE 	= ClassiCube
APP_AUTHOR 	= ClassiCube team
APP_ICON    = misc/switch/icon.jpg
APP_VERSION	= 1.3.7


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

ARCH	= -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS	= -g -Wall -O2 -ffunction-sections $(ARCH) -D__SWITCH__
ASFLAGS	= -g $(ARCH)

LDFLAGS	= -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH)
LIBS	= -lGLESv2 -lEGL -lglapi -ldrm_nouveau -lnx -lm

LIBNX		= $(DEVKITPRO)/libnx
INCLUDES 	+= $(foreach dir, $(LIBNX), -I$(dir)/include)
LDFLAGS 	+= $(foreach dir, $(LIBNX), -L$(dir)/lib)

PORTLIBS	= $(PORTLIBS_PATH)/switch
INCLUDES 	+= $(foreach dir, $(PORTLIBS), -I$(dir)/include)
LDFLAGS 	+= $(foreach dir, $(PORTLIBS), -L$(dir)/lib)

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


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
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/$(TARGET).nacp:
	nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@ --titleid=$(APP_TITLEID)

$(TARGET).nro: $(TARGET).elf $(BUILD_DIR)/$(TARGET).nacp
	elf2nro $< $@ --icon=$(APP_ICON) --nacp=$(BUILD_DIR)/$(TARGET).nacp


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/switch/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
