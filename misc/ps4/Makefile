ifeq ($(strip $(OO_PS4_TOOLCHAIN)),)
$(error "Please set OO_PS4_TOOLCHAIN in your environment. export OO_PS4_TOOLCHAIN=<path>")
endif
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)

.SUFFIXES:

#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
BUILD_DIR	= build/ps4
SOURCE_DIRS = src src/ps4 third_party/bearssl

TARGET		= ClassiCube-PS4
TITLE       := ClassiCube
VERSION     := 1.38
TITLE_ID    := CUBE00200
CONTENT_ID  := IV0000-CUBE00200_00-CLASSICUBE000000

#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
CPP_FILES 	= $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.cpp))
C_FILES 	= $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 		= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(CPP_FILES:%.cpp=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)

#---------------------------------------------------------------------------------
# Code generation
#--------------------------------------------------------------------------------
LIBS        := -lc -lkernel -lc++ -lSceVideoOut -lSceNet -lScePad -lSceUserService -lSceRtc

CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/orbis -DPLAT_PS4
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

SYS_DIR		:= $(BUILD_DIR)/sce_sys
MODS_DIR	:= $(BUILD_DIR)/sce_module


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC      := clang
CCX     := clang++
LD      := ld.lld
CDIR    := linux

GP4TOOL	:= $(TOOLCHAIN)/bin/$(CDIR)/create-gp4
PKGTOOL := $(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core
MKFSELF := $(TOOLCHAIN)/bin/$(CDIR)/create-fself


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR) $(TARGET).pkg

clean:
	rm -f $(CONTENT_ID).pkg pkg.gp4 pkg/sce_sys/param.sfo eboot.bin \
		$(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).self $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).pkg: $(BUILD_DIR)/$(CONTENT_ID).pkg
	mv $(BUILD_DIR)/$(CONTENT_ID).pkg $(TARGET).pkg

$(BUILD_DIR)/$(CONTENT_ID).pkg: $(BUILD_DIR)/pkg.gp4
	$(PKGTOOL) pkg_build $< $(BUILD_DIR)

$(BUILD_DIR)/pkg.gp4: $(BUILD_DIR)/eboot.bin $(SYS_DIR)/param.sfo $(SYS_DIR)/icon0.png $(SYS_DIR)/about/right.sprx $(MODS_DIR)/libSceFios2.prx $(MODS_DIR)/libc.prx
	cd $(BUILD_DIR)
	$(GP4TOOL) -out $@ --content-id=$(CONTENT_ID) --files \
		"eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx"

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(LD) $(OBJS) -o $(BUILD_DIR)/$(TARGET).elf $(LDFLAGS)

$(BUILD_DIR)/eboot.bin: $(BUILD_DIR)/$(TARGET).elf
	$(MKFSELF) -in=$(BUILD_DIR)/$(TARGET).elf -out=$(BUILD_DIR)/$(TARGET).self --eboot "$(BUILD_DIR)/eboot.bin" --paid 0x3800000000000011


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
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	$(CCX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/ps4/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	$(CC) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
