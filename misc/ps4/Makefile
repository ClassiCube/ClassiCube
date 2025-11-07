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
VERSION     := 1.00
TITLE_ID    := BREW00083
CONTENT_ID  := IV0000-BREW00083_00-HELLOWORLD000000

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
LIBS        := -lc -lkernel -lc++ -lSceVideoOut -lSceNet

CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/orbis -DPLAT_PS4
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC      := clang
CCX     := clang++
LD      := ld.lld
CDIR    := linux


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR) $(CONTENT_ID).pkg

clean:
	rm -f $(CONTENT_ID).pkg pkg.gp4 pkg/sce_sys/param.sfo eboot.bin \
		$(BUILD_DIR)/$(PROJDIR).elf $(BUILD_DIR)/$(PROJDIR).oelf $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(CONTENT_ID).pkg: $(BUILD_DIR)/pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

$(BUILD_DIR)/pkg.gp4: $(BUILD_DIR)/eboot.bin $(BUILD_DIR)/sce_sys/param.sfo $(BUILD_DIR)/sce_sys/icon0.png $(BUILD_DIR)/sce_sys/about/right.sprx
	cd $(BUILD_DIR)
	$(TOOLCHAIN)/bin/$(CDIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png"

$(BUILD_DIR)/sce_sys/icon0.png: misc/ps4/icon0.png
	cp $< $@

$(BUILD_DIR)/sce_sys/about/right.sprx: misc/ps4/right.sprx
	mkdir -p $(BUILD_DIR)/sce_sys/about
	cp $< $@

$(BUILD_DIR)/sce_sys/param.sfo:
	mkdir -p $(BUILD_DIR)/sce_sys
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1 
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0  
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'  
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0 
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0  
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(LD) $(OBJS) -o $(BUILD_DIR)/$(TARGET).elf $(LDFLAGS)

$(BUILD_DIR)/eboot.bin: $(BUILD_DIR)/$(TARGET).elf
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(BUILD_DIR)/$(TARGET).elf -out=$(BUILD_DIR)/$(TARGET).oelf --eboot "$(BUILD_DIR)/eboot.bin" --paid 0x3800000000000011


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
