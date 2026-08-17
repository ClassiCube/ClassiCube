ifeq ($(strip $(PSPDEV)),)
$(warning "Please set PSPDEV variables in your environment. For example:")
$(warning   export PSPDEV=/usr/local/pspsdk)
$(error   Failed to find PSPSDK installation)
endif
PSPSDK=$(PSPDEV)/psp/sdk

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET 		:= ClassiCube-psp
SOURCE_DIRS	:= src src/psp third_party/bearssl
BUILD_DIR	:= build/psp
RUN_PROGRAM	?= ppsspp

SFO_TITLE  := ClassiCube
SFO_DISCID := CUBE02000

EBOOT_ICON	 := misc/psp/ICON0.png
EBOOT_ICON1  := NULL
EBOOT_UNKPNG := NULL
EBOOT_PIC1   := NULL
EBOOT_SND0   := NULL
EBOOT_PSAR   := NULL

PARAM_SFO    := $(BUILD_DIR)/param.sfo
EBOOT_PBP    := EBOOT.PBP

default: $(EBOOT_PBP)


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS   := -I$(PSPSDK)/include -g -O2 -fno-math-errno -D_PSP_FW_VERSION=600 -DPLAT_PSP

LDFLAGS  := -L$(PSPSDK)/lib -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx -Wl,-zmax-page-size=128
LIBS 	 := -lm -lpspgu -lpspge -lpspdisplay -lpspctrl -lpspnet -lpspnet_apctl


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC       := $(PSPDEV)/bin/psp-gcc
LINK     := $(CC)

MKSFOEX  := $(PSPDEV)/bin/mksfoex
PACK_PBP := $(PSPDEV)/bin/pack-pbp
FIXUP    := $(PSPDEV)/bin/psp-fixup-imports
PRXGEN   := $(PSPDEV)/bin/psp-prxgen


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(EBOOT_PBP) $(PARAM_SFO) $(TARGET).prx $(TARGET)-fixed.elf
include misc/makefiles/common_build.mk

$(TARGET)-fixed.elf: $(TARGET).elf
	$(FIXUP) -o $@ $<

$(TARGET).prx: $(TARGET)-fixed.elf
	$(PRXGEN) $< $@

$(PARAM_SFO): 
	$(MKSFOEX) -d MEMSIZE=1 -s DISC_ID="$(SFO_DISCID)" '$(SFO_TITLE)' $

$(EBOOT_PBP): $(TARGET).prx $(PARAM_SFO)
	$(PACK_PBP) $@ $(PARAM_SFO) $(EBOOT_ICON) \
		$(EBOOT_ICON1) $(EBOOT_UNKPNG) $(EBOOT_PIC1) \
		$(EBOOT_SND0)  $(TARGET).prx $(EBOOT_PSAR)


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
