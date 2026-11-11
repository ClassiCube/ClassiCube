ifeq ($(strip $(XSDK_TOOLCHAIN)),)
$(error "Please set XSDK_TOOLCHAIN in your environment. export XSDK_TOOLCHAIN=<path to>toolchain")
endif
ifeq ($(strip $(XSDK_SDK)),)
$(error "Please set XSDK_SDK in your environment. export XSDK_SDK=<path to>toolchain")
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		:= ClassiCube-xbox360
BUILD_DIR	:= build/xbox360
SOURCE_DIRS	:= src src/xbox360 third_party/bearssl
RUN_PROGRAM	?= xenon

default: $(TARGET).xex


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS	:= -isystem $(XSDK_SDK)/include -mcpu=cell -fno-pic -O2 -g -DPLAT_XBOX360

LDFLAGS	:= -g -static -nostdlib -T $(XSDK_SDK)/src/xsdk.ld 
LIBS	:= -Wl,--start-group $(XSDK_SDK)/build/libxsdk.a -lc -lm -lgcc -Wl,--end-group


#---------------------------------------------------------------------------------
# Tools
#---------------------------------------------------------------------------------
CC 		:= $(XSDK_TOOLCHAIN)/bin/powerpc-xsdk-elf-gcc
OBJDUMP	:= $(XSDK_TOOLCHAIN)/bin/powerpc-xsdk-elf-objdump
LINK	:= $(CC)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .elf
GEN_FILES 	:= $(TARGET).xex
include misc/makefiles/common_build.mk

$(TARGET).xex: $(TARGET).elf
	sh -c "'$(OBJDUMP)' -d '$<' | python3 '$(XSDK_SDK)/tools/xenia_scan.py'"
	python3 $(XSDK_SDK)/tools/elf2xex.py $< $@


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
