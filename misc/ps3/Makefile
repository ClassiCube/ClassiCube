ifeq ($(strip $(PS3DEV)),)
$(error "Please set PS3DEV in your environment. export PS3DEV=<path>")
endif
ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. export PSL1GHT=<path>")
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		:= ClassiCube-PS3
BUILD_DIR	:= build/ps3
SOURCE_DIRS := src src/ps3 third_party/bearssl
RUN_PROGRAM	?= rpcs3

TITLE		:= ClassiCube
APPID		:= CUBE00200
CONTENTID	:= UP0001-$(APPID)_00-0000000000000000
ICON0		:= misc/ps3/ICON0.png
SFOXML		:= misc/ps3/sfo.xml


#---------------------------------------------------------------------------------
# Code generation
#--------------------------------------------------------------------------------
MACHDEP	= -mhard-float -fmodulo-sched -ffunction-sections -fdata-sections

CFLAGS	= -O2 -DPLAT_PS3 -Wall -mcpu=cell -fno-math-errno $(MACHDEP) \
			-I$(PSL1GHT)/ppu/include -I$(PSL1GHT)/ppu/include/simdmath
LDFLAGS	= $(MACHDEP) -fno-use-linker-plugin -L$(PSL1GHT)/ppu/lib
LIBS	= -lrsx -lgcm_sys -lio -lsysutil -lrt -llv2 -lm -lnet -lsysmodule 


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
AS		:=	$(PS3DEV)/ppu/bin/ppu-as
CC		:=	$(PS3DEV)/ppu/bin/ppu-gcc
STRIP	:=	$(PS3DEV)/ppu/bin/ppu-strip
LINK	:=	$(CC)

PKG		:=	$(PS3DEV)/bin/pkg.py
SFO		:=	$(PS3DEV)/bin/sfo.py

SPRX	:=	$(PS3DEV)/bin/sprxlinker
CGCOMP	:=	$(PS3DEV)/bin/cgcomp

# fake SELF type4 / type8 tools
FSELF			:=	$(PS3DEV)/bin/fself
FSELF_NPDRM		:=	$(PS3DEV)/bin/fself -n

# signed SELF type4 / type8 tools
SELF			:=	$(PS3DEV)/bin/make_self
SELF_NPDRM		:=	$(PS3DEV)/bin/make_self_npdrm

# NPDRM pkg tool
PACKAGE_FINALIZE :=	$(PS3DEV)/bin/package_finalize


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
default: $(TARGET).gnpdrm.pkg $(TARGET).self $(TARGET).fake.self

$(TARGET).gnpdrm.pkg: $(TARGET).pkg
	cp $< $@
	$(PACKAGE_FINALIZE) $@

$(TARGET).pkg: $(TARGET).sprx
	mkdir -p $(BUILD_DIR)/pkg/USRDIR
	cp $(ICON0) $(BUILD_DIR)/pkg/ICON0.PNG
	$(SELF_NPDRM) $< $(BUILD_DIR)/pkg/USRDIR/EBOOT.BIN $(CONTENTID)
	$(SFO) --title "$(TITLE)" --appid "$(APPID)" -f $(SFOXML) $(BUILD_DIR)/pkg/PARAM.SFO
	$(PKG) --contentid $(CONTENTID) $(BUILD_DIR)/pkg/ $@

$(TARGET).self: $(TARGET).sprx
	$(SELF) $< $@

$(TARGET).sprx: $(TARGET).elf
	$(STRIP) $< -o $@
	$(SPRX) $@

$(TARGET).fake.self: $(TARGET).elf
	$(FSELF) $< $@


OEXT		:= .elf
GEN_FILES 	:= $(TARGET).fake.self $(TARGET).sprx $(TARGET).self $(TARGET).pkg $(TARGET).gnpdrm.pkg
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# Shader generation
#---------------------------------------------------------------------------------
SHDR_DIR	:= misc/ps3
VCGFILES	:= $(notdir $(wildcard $(SHDR_DIR)/*.vcg))
FCGFILES	:= $(notdir $(wildcard $(SHDR_DIR)/*.fcg))
VPOFILES	:= $(addprefix $(BUILD_DIR)/, $(VCGFILES:.vcg=.vpo))
FPOFILES	:= $(addprefix $(BUILD_DIR)/, $(FCGFILES:.fcg=.fpo))

SHDR_OBJS	:= $(addsuffix .o, $(VPOFILES)) $(addsuffix .o, $(FPOFILES))
OBJECTS 	+= $(SHDR_OBJS)

src/ps3/Graphics_PS3.c: $(SHDR_OBJS)


# prevent the .asm files from being deleted
.PRECIOUS: $(BUILD_DIR)/%.vpo $(BUILD_DIR)/%.fpo

$(BUILD_DIR)/%.vpo: $(SHDR_DIR)/%.vcg
	$(CGCOMP) -v $(CGCFLAGS) $^ $@
$(BUILD_DIR)/%.fpo: $(SHDR_DIR)/%.fcg
	$(CGCOMP) -f $(CGCFLAGS) $^ $@

$(BUILD_DIR)/%.vpo.o: $(BUILD_DIR)/%.vpo
	bin2s -a 64 $< | $(AS) -o $@
$(BUILD_DIR)/%.fpo.o: $(BUILD_DIR)/%.fpo
	bin2s -a 64 $< | $(AS) -o $@


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
