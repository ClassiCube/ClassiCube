ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

include $(DEVKITPRO)/devkitARM/base_tools


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 		= ClassiCube-3ds
# List of directories containing source code
SOURCE_DIRS	= src src/3ds third_party/bearssl/src
# List of directories containing shader files
SHDR_DIRS   = misc/3ds
# List of directories containing more header files
INCLUDES	= -Ithird_party/bearssl/inc
# Directory where object files are placed
BUILD_DIR	= build/3ds

APP_ICON	= misc/3ds/icon.png
APP_TITLE 	= ClassiCube
APP_DESCRIPTION = Simple block building sandbox
APP_AUTHOR 	= ClassiCube team

CIA_BANNER_BIN	= misc/3ds/banner.bin
CIA_ICON_BIN	= misc/3ds/icon.bin
CIA_SPEC_RSF	= misc/3ds/spec.rsf


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

PICAFILES 	:= $(foreach dir,$(SHDR_DIRS),$(notdir $(wildcard $(dir)/*.v.pica)))
OBJS		+= $(addprefix $(BUILD_DIR)/, $(PICAFILES:.v.pica=.shbin.o))

ARCH	= -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS	= -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
ASFLAGS	= -g $(ARCH)

LDFLAGS	= -specs=3dsx.specs -g $(ARCH)
LIBS	= -lctru -lm

CTRULIB		= $(DEVKITPRO)/libctru
INCLUDES 	+= $(foreach dir, $(CTRULIB), -I$(dir)/include)
LDFLAGS 	+= $(foreach dir, $(CTRULIB), -L$(dir)/lib)

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).cia

clean:
	rm $(TARGET).cia $(TARGET).3dsx $(TARGET).elf $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR).smdh: $(APP_ICON)
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(APP_ICON) $@

$(TARGET).3dsx: $(TARGET).elf $(BUILD_DIR).smdh
	3dsxtool $< $@ --smdh=$(BUILD_DIR).smdh

$(BUILD_DIR)/makerom:
	wget https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.18.3/makerom-v0.18.3-ubuntu_x86_64.zip -O $(BUILD_DIR)/makerom.zip
	unzip $(BUILD_DIR)/makerom.zip -d $(BUILD_DIR)
	chmod +x $(BUILD_DIR)/makerom

$(TARGET).cia : $(TARGET).3dsx $(BUILD_DIR)/makerom
	$(BUILD_DIR)/makerom -f cia -o $(TARGET).cia -elf $(TARGET).elf -rsf $(CIA_SPEC_RSF) -icon $(CIA_ICON_BIN) -banner $(CIA_BANNER_BIN) -exefslogo -target t


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/3ds/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.shbin: misc/3ds/%.v.pica
	picasso $< -o $@

$(BUILD_DIR)/%.shbin.o: $(BUILD_DIR)/%.shbin
	bin2s $< | $(AS) -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
