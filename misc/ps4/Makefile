ifeq ($(strip $(OO_PS4_TOOLCHAIN)),)
$(error "Please set OO_PS4_TOOLCHAIN in your environment. export OO_PS4_TOOLCHAIN=<path>")
endif
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)

.SUFFIXES:

#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		:= ClassiCube-PS4
SOURCE_DIRS := src src/ps4 third_party/bearssl
BUILD_DIR	:= build/ps4
RUN_PROGRAM	?= shadps4

TITLE       := ClassiCube
VERSION     := 1.38
TITLE_ID    := CUBE00200
CONTENT_ID  := IV0000-CUBE00200_00-CLASSICUBE000000

default: $(TARGET).pkg


#---------------------------------------------------------------------------------
# Code generation
#--------------------------------------------------------------------------------
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/orbis -DPLAT_PS4
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1

LIBS        := -lc -lkernel -lc++ -lSceVideoOut -lSceNet -lScePad -lSceUserService -lSceRtc
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(TOOLCHAIN)/lib/crt1.o

SYS_DIR		:= $(BUILD_DIR)/sce_sys
MODS_DIR	:= $(BUILD_DIR)/sce_module


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC		:= clang
CXX		:= clang++
LINK	:= ld.lld
CDIR	:= linux

GP4TOOL	:= DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=true $(TOOLCHAIN)/bin/$(CDIR)/create-gp4
PKGTOOL := DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=true $(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core
MKFSELF := $(TOOLCHAIN)/bin/$(CDIR)/create-fself


#---------------------------------------------------------------------------------
# Support files
#---------------------------------------------------------------------------------
$(SYS_DIR)/icon0.png: misc/ps4/icon0.png
	cp $< $@

$(SYS_DIR)/about/right.sprx: misc/ps4/right.sprx
	mkdir -p $(SYS_DIR)/about
	cp $< $@

$(SYS_DIR)/param.sfo:
	mkdir -p $(SYS_DIR)
	$(PKGTOOL) sfo_new $@
	$(PKGTOOL) sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOL) sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOL) sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOL) sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOL) sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOL) sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOL) sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'


$(MODS_DIR)/libSceFios2.prx: misc/ps4/sce_module/libSceFios2.prx
	mkdir -p $(MODS_DIR)
	cp $< $@

$(MODS_DIR)/libc.prx: misc/ps4/sce_module/libc.prx
	mkdir -p $(MODS_DIR)
	cp $< $@


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).pkg $(BUILD_DIR)/$(CONTENT_ID).pkg $(BUILD_DIR)/pkg.gp4 \
				$(BUILD_DIR)/$(TARGET).self $(BUILD_DIR)/eboot.bin $(SYS_DIR)/param.sfo
include misc/makefiles/common_build.mk

$(BUILD_DIR)/eboot.bin: $(TARGET).elf
	$(MKFSELF) -in=$< -out=$(BUILD_DIR)/$(TARGET).self --eboot "$(BUILD_DIR)/eboot.bin" --paid 0x3800000000000011

$(BUILD_DIR)/pkg.gp4: $(BUILD_DIR)/eboot.bin $(SYS_DIR)/param.sfo $(SYS_DIR)/icon0.png $(SYS_DIR)/about/right.sprx $(MODS_DIR)/libSceFios2.prx $(MODS_DIR)/libc.prx
	cd $(BUILD_DIR)
	$(GP4TOOL) -out $@ --content-id=$(CONTENT_ID) --files \
		"eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx"

$(BUILD_DIR)/$(CONTENT_ID).pkg: $(BUILD_DIR)/pkg.gp4
	$(PKGTOOL) pkg_build $< $(BUILD_DIR)

$(TARGET).pkg: $(BUILD_DIR)/$(CONTENT_ID).pkg
	mv $< $@


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
