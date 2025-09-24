ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 		= ClassiCube-wiiu
# List of directories containing source code
SOURCE_DIRS	= src src/wiiu third_party/bearssl
# Directory where object files are placed
BUILD_DIR	= build/wiiu
# Directory where shader files are
SHADERS		= misc/wiiu

WUHB_OPTIONS = \
	--name "ClassiCube" \
	--short-name "ClassiCube" \
	--author "ClassiCube team" \
	--icon=misc/wiiu/icon.png

#WUHB_OPTIONS += --tv-image=misc/wiiu/tv_splash.png
#WUHB_OPTIONS += --drc-image=misc/wiiu/drc_splash.png


#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

BINFILES :=	$(foreach dir,$(SHADERS),$(wildcard $(dir)/*.gsh))
OBJS 	 += $(addprefix $(BUILD_DIR)/, $(notdir $(BINFILES:%.gsh=%.gsh.o)))

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
MACHDEP	= -DESPRESSO -mcpu=750 -meabi -mhard-float
CFLAGS	= -g -Wall -O2 -ffunction-sections -fno-math-errno $(MACHDEP) \
			-I $(DEVKITPRO)/wut/include \
			-D__WIIU__ -D__WUT__ -DPLAT_WIIU

LDFLAGS	= -g $(MACHDEP) -specs=$(DEVKITPRO)/wut/share/wut.specs \
			-L $(DEVKITPRO)/wut/lib
# Additional libraries to link against
LIBS	= -lwut -lm


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(DEVKITPRO)/devkitPPC/bin/powerpc-eabi-
PPC_AS	:= $(PREFIX)as
PPC_CC	:= $(PREFIX)gcc
PPC_CXX	:= $(PREFIX)g++

ELF2RPL	 := $(DEVKITPRO)/tools/bin/elf2rpl
WUHBTOOL := $(DEVKITPRO)/tools/bin/wuhbtool
BIN2S	 := $(DEVKITPRO)/tools/bin/bin2s


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).wuhb

clean:
	rm $(TARGET).wuhb $(TARGET).rpx $(TARGET).elf $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(PPC_CC) $(LDFLAGS) $^ -o $@ $(LIBS)
	
$(TARGET).rpx: $(TARGET).elf
	$(ELF2RPL) $< $@
	
$(TARGET).wuhb: $(TARGET).rpx
	$(WUHBTOOL) $< $@ $(WUHB_OPTIONS)


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(PPC_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/wiiu/%.S
	$(PPC_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/wiiu/%.c
	$(PPC_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	$(PPC_CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.gsh.o : misc/wiiu/%.gsh
	$(BIN2S) $< | $(PPC_CC) -x assembler-with-cpp -c - -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
