PROJECT_TITLE 	:= ClassiCube
PROJECT_TITLEID := CUBE00200
TARGET 			:= ClassiCube-vita

BUILD_DIR		:= build-vita
SOURCE_DIRS		:= src third_party/bearssl/src

CFLAGS += -Wl,-q -Ithird_party/bearssl/inc -O1
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o)))

LIBS += -lm -lSceDisplay_stub -lSceCtrl_stub -lSceTouch_stub -lSceGxm_stub -lSceCommonDialog_stub -lSceAppUtil_stub

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)

ifeq ($(strip $(VITASDK)),)
$(warning "Please set VITASDK variables in your environment. For example:")
$(warning   export VITASDK=/usr/local/vitasdk)
$(warning   export PATH=$$VITASDK/bin:$$PATH)
$(error   Failed to find VitaSDK installation)
endif


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).vpk

clean:
	rm $(TARGET).velf $(TARGET).elf $(TARGET).vpk param.sfo eboot.bin $(OBJS)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).vpk: eboot.bin param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin -a misc/vita/sce_sys/ICON0.png=sce_sys/ICON0.png $(TARGET).vpk

eboot.bin: $(TARGET).velf
	vita-make-fself $(TARGET).velf eboot.bin

param.sfo:
	vita-mksfoex -s TITLE_ID="$(PROJECT_TITLEID)" "$(PROJECT_TITLE)" param.sfo

$(TARGET).velf: $(TARGET).elf
	arm-vita-eabi-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	arm-vita-eabi-gcc $(CFLAGS) $^ $(LIBS) -o $@


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o : src/%.c
	arm-vita-eabi-gcc $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o : third_party/bearssl/src/%.c
	arm-vita-eabi-gcc $(CFLAGS) -c $< -o $@

# Dependency tracking
$(DEPFILES):

include $(wildcard $(DEPFILES))
