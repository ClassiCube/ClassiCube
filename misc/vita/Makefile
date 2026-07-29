ifeq ($(strip $(VITASDK)),)
$(warning "Please set VITASDK variables in your environment. For example:")
$(warning   export VITASDK=/usr/local/vitasdk)
$(error   Failed to find VitaSDK installation)
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-vita
SOURCE_DIRS	:= src src/psvita third_party/bearssl
BUILD_DIR	:= build/vita
RUN_PROGRAM	?= vita3k

SFO_TITLE 	:= ClassiCube
SFO_TITLEID := CUBE00200

PARAM_SFO	:= $(BUILD_DIR)/param.sfo
EBOOT_BIN	:= eboot.bin


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS 	:= -O2 -DPLAT_VITA -fno-math-errno

LDFLAGS := -Wl,-q
LIBS 	:= -lm -lSceDisplay_stub -lSceCtrl_stub -lSceTouch_stub -lSceGxm_stub -lSceCommonDialog_stub -lSceAppUtil_stub


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
PREFIX	:= $(VITASDK)/bin/arm-vita-eabi-
CC		:= $(PREFIX)gcc
STRIP 	:= $(PREFIX)strip
LINK	:= $(CC)

PACK_VPK 	:= $(VITASDK)/bin/vita-pack-vpk
MAKE_FSELF	:= $(VITASDK)/bin/vita-make-fself
MKSOFEX		:= $(VITASDK)/bin/vita-mksfoex
ELF_CREATE	:= $(VITASDK)/bin/vita-elf-create


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).vpk: $(EBOOT_BIN) $(PARAM_SFO)
	$(PACK_VPK) -s $(PARAM_SFO) -b $(EBOOT_BIN) -a misc/vita/ICON0.png=sce_sys/ICON0.png $@

$(EBOOT_BIN): $(TARGET).velf
	$(MAKE_FSELF) $< $@

$(PARAM_SFO):
	$(MKSOFEX) -s TITLE_ID="$(SFO_TITLEID)" "$(SFO_TITLE)" $@

$(TARGET).velf: $(TARGET).stripped
	$(ELF_CREATE) $< $@

$(TARGET).stripped: $(TARGET).elf
	$(STRIP) -g $< -o $@


OEXT		:= .elf
GEN_FILES 	:= $(TARGET).vpk $(EBOOT_BIN) $(PARAM_SFO) $(TARGET).stripped $(TARGET).velf
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# Shader generation
#---------------------------------------------------------------------------------
SHDR_DIR	:= misc/vita
SHDR_SRCS 	:= $(notdir $(wildcard $(SHDR_DIR)/*.gxp))
SHDR_OBJS	:= $(addprefix $(BUILD_DIR)/, $(SHDR_SRCS:.gxp=.gxp.h))
GEN_FILES	+= $(SHDR_OBJS)

$(BUILD_DIR)/%.gxp.h: $(SHDR_DIR)/%.gxp
	xxd -i $< > $@

src/psvita/Graphics_PSVita.c: $(SHDR_OBJS)
