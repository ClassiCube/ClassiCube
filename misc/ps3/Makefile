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
# Name of the final output
TARGET		= ClassiCube-PS3
# Directory where object files are placed
BUILD_DIR	= build/ps3
# List of directories containing source code
SOURCE_DIRS = src src/ps3 third_party/bearssl
SHADERS		= misc/ps3

TITLE		= ClassiCube
APPID		= CUBE00200
CONTENTID	= UP0001-$(APPID)_00-0000000000000000
ICON0		= misc/ps3/ICON0.png
SFOXML		= misc/ps3/sfo.xml


#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)

# Shaders
VCGFILES	:= $(notdir $(wildcard $(SHADERS)/*.vcg))
FCGFILES	:= $(notdir $(wildcard $(SHADERS)/*.fcg))
VPOFILES	:= $(addprefix $(BUILD_DIR)/, $(VCGFILES:.vcg=.vpo))
FPOFILES	:= $(addprefix $(BUILD_DIR)/, $(FCGFILES:.fcg=.fpo))

OBJS 		+= $(addsuffix .o, $(VPOFILES))
OBJS 		+= $(addsuffix .o, $(FPOFILES))


#---------------------------------------------------------------------------------
# Code generation
#--------------------------------------------------------------------------------
MACHDEP	= -mhard-float -fmodulo-sched -ffunction-sections -fdata-sections

CFLAGS	= -O2 -DPLAT_PS3 -Wall -mcpu=cell -fno-math-errno $(MACHDEP) $(INCLUDE)
LDFLAGS	= $(MACHDEP) -Wl,-Map,$(notdir $@).map -fno-use-linker-plugin
LIBS	= -lrsx -lgcm_sys -lio -lsysutil -lrt -llv2 -lm -lnet -lsysmodule 

INCLUDE = -I$(PSL1GHT)/ppu/include -I$(PSL1GHT)/ppu/include/simdmath
LIBPATHS = -L$(PSL1GHT)/ppu/lib


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
AS		:=	$(PS3DEV)/ppu/bin/ppu-as
CC		:=	$(PS3DEV)/ppu/bin/ppu-gcc
LD		:=	$(PS3DEV)/ppu/bin/ppu-gcc
STRIP	:=	$(PS3DEV)/ppu/bin/ppu-strip
OBJCOPY	:=	$(PS3DEV)/ppu/bin/ppu-objcopy

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
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).pkg $(TARGET).fake.self

clean:
	rm -f $(TARGET).elf $(TARGET).fake.self $(TARGET).sprx $(TARGET).self $(TARGET).pkg $(OBJS) $(DEPFILES)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(LD) $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@

$(TARGET).fake.self: $(TARGET).elf
	$(FSELF) $< $@

$(TARGET).sprx: $(TARGET).elf
	$(STRIP) $< -o $@
	$(SPRX) $@

$(TARGET).self: $(TARGET).sprx
	$(SELF) $< $@

$(TARGET).pkg: $(TARGET).self
	mkdir -p $(BUILD_DIR)/pkg/USRDIR
	cp $(ICON0) $(BUILD_DIR)/pkg/ICON0.PNG
	$(SELF_NPDRM) $(TARGET).sprx $(BUILD_DIR)/pkg/USRDIR/EBOOT.BIN $(CONTENTID) >> /dev/null
	$(SFO) --title "$(TITLE)" --appid "$(APPID)" -f $(SFOXML) $(BUILD_DIR)/pkg/PARAM.SFO
	$(PKG) --contentid $(CONTENTID) $(BUILD_DIR)/pkg/ $@ >> /dev/null
	cp $@ $(TARGET).gnpdrm.pkg
	$(PACKAGE_FINALIZE) $(TARGET).gnpdrm.pkg


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/ps3/%.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/ps3/%.S
	$(CC) $(CFLAGS) $(INCLUDE) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@


# prevent the .asm files from being deleted
.PRECIOUS: $(BUILD_DIR)/%.vpo $(BUILD_DIR)/%.fpo

$(BUILD_DIR)/%.vpo: $(SHADERS)/%.vcg
	$(CGCOMP) -v $(CGCFLAGS) $^ $@
$(BUILD_DIR)/%.fpo: $(SHADERS)/%.fcg
	$(CGCOMP) -f $(CGCFLAGS) $^ $@

$(BUILD_DIR)/%.vpo.o: $(BUILD_DIR)/%.vpo
	bin2s -a 64 $< | $(AS) -o $@
$(BUILD_DIR)/%.fpo.o: $(BUILD_DIR)/%.fpo
	bin2s -a 64 $< | $(AS) -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
