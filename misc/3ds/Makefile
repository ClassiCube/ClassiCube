#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# configurable options
#---------------------------------------------------------------------------------
APP_ICON	:= $(TOPDIR)/misc/3ds/icon.png
APP_TITLE 	:= ClassiCube
APP_DESCRIPTION := Simple block building sandbox
APP_AUTHOR 	:= UnknownShadow200
TARGET 		:= ClassiCube-3ds

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
BUILD		:=	build-3ds
SOURCES		:= src misc/3ds third_party/bearssl/src
INCLUDES	:=	third_party/bearssl/inc

CIA_BANNER_BIN	:= $(TOPDIR)/misc/3ds/banner.bin
CIA_ICON_BIN	:= $(TOPDIR)/misc/3ds/icon.bin
CIA_SPEC_RSF	:= $(TOPDIR)/misc/3ds/spec.rsf

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lctru -lm


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))

export LD	:=	$(CC)

export OFILES_SOURCES 	:=	$(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(PICAFILES:.v.pica=.shbin.o)

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(CTRULIB),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(CTRULIB),-L$(dir)/lib)

export _3DSXFLAGS = --smdh=$(CURDIR)/$(TARGET).smdh

.PHONY: all clean

#---------------------------------------------------------------------------------
all: $(BUILD)
	$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/misc/3ds/Makefile

$(BUILD):
	mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	echo clean ...
	rm -fr $(BUILD) $(TARGET).cia $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).cia	: $(OUTPUT).3dsx makerom
	./makerom -f cia -o "$(OUTPUT).cia" -elf "$(OUTPUT).elf" -rsf "$(CIA_SPEC_RSF)" -icon "$(CIA_ICON_BIN)" -banner "$(CIA_BANNER_BIN)" -exefslogo -target t
	
makerom:
	wget https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.18.3/makerom-v0.18.3-ubuntu_x86_64.zip
	unzip makerom-v0.18.3-ubuntu_x86_64.zip
	chmod +x makerom

$(OUTPUT).3dsx	:	$(OUTPUT).elf $(OUTPUT).smdh

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
.PRECIOUS	:	%.shbin
#---------------------------------------------------------------------------------

%.shbin.o : %.v.pica
	$(eval CUR_SRC	:= $<)
	$(eval CUR_FILE := $(notdir $(CUR_SRC)))
	$(eval CUR_BIN	:= $(CURDIR)/$(subst .v.pica,.shbin,$(CUR_FILE)))
	picasso -o $(CUR_BIN) $(CUR_SRC)
	bin2s $(CUR_BIN) | $(AS) -o $(CUR_BIN).o


-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
