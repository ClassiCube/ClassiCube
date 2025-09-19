ifeq ($(strip $(VITASDK)),)
$(warning "Please set VITASDK variables in your environment. For example:")
$(warning   export VITASDK=/usr/local/vitasdk)
$(error   Failed to find VitaSDK installation)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 			= ClassiCube-vita
# List of directories containing source code
SOURCE_DIRS		= src src/psvita third_party/bearssl
# Directory where object files are placed
BUILD_DIR		= build/vita

SFO_TITLE 	= ClassiCube
SFO_TITLEID = CUBE00200


#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS 	= -O1 -DPLAT_VITA -fno-math-errno

LDFLAGS = -Wl,-q
LIBS 	= -lm -lSceDisplay_stub -lSceCtrl_stub -lSceTouch_stub -lSceGxm_stub -lSceCommonDialog_stub -lSceAppUtil_stub


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(VITASDK)/bin/arm-vita-eabi-
VITA_AS	:= $(PREFIX)as
VITA_CC	:= $(PREFIX)gcc
VITA_CXX:= $(PREFIX)g++
VITA_STRIP := $(PREFIX)strip

PACK_VPK 	:= $(VITASDK)/bin/vita-pack-vpk
MAKE_FSELF	:= $(VITASDK)/bin/vita-make-fself
MKSOFEX		:= $(VITASDK)/bin/vita-mksfoex
ELF_CREATE	:= $(VITASDK)/bin/vita-elf-create


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).vpk

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm $(TARGET).velf $(TARGET).elf $(TARGET).vpk param.sfo eboot.bin $(OBJS)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).vpk: eboot.bin param.sfo
	$(PACK_VPK) -s param.sfo -b eboot.bin -a misc/vita/sce_sys/ICON0.png=sce_sys/ICON0.png $(TARGET).vpk

eboot.bin: $(TARGET).velf
	$(MAKE_FSELF) $(TARGET).velf eboot.bin

param.sfo:
	$(MKSOFEX) -s TITLE_ID="$(SFO_TITLEID)" "$(SFO_TITLE)" param.sfo

$(TARGET).velf: $(TARGET).elf
	$(VITA_STRIP) -g $<
	$(ELF_CREATE) $< $@

$(TARGET).elf: $(OBJS)
	$(VITA_CC) $(LDFLAGS) $^ $(LIBS) -o $@


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o : src/%.c
	$(VITA_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : src/psvita/%.c
	$(VITA_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o : third_party/bearssl/%.c
	$(VITA_CC) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
