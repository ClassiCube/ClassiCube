ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON in your environment. export DEVKITXENON=<path to>devkitPPC")
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		:= ClassiCube-xbox360
BUILD_DIR	:= build/xbox360
SOURCE_DIRS	:= src src/xbox360 third_party/bearssl
RUN_PROGRAM	?= xenon


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
MACHDEP := -DXENON -m32 -maltivec -fno-pic -mpowerpc64 -mhard-float

CFLAGS	:= -g -O2 -Wall $(MACHDEP) \
			-I$(DEVKITXENON)/usr/include

LDFLAGS	:= -g $(MACHDEP) -Wl,--gc-sections \
			-L$(DEVKITXENON)/usr/lib \
			-L$(DEVKITXENON)/xenon/lib/32 \
			-n -T $(DEVKITXENON)/app.lds
LIBS	:= -lxenon -lm -lfat


#---------------------------------------------------------------------------------
# Tools
#---------------------------------------------------------------------------------
CC 		:= $(DEVKITXENON)/bin/xenon-gcc
OBJCOPY	:= $(DEVKITXENON)/bin/xenon-objcopy
STRIP	:= $(DEVKITXENON)/bin/xenon-strip
LINK	:= $(CC)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf32: $(TARGET).elf
	$(OBJCOPY) -O elf32-powerpc --adjust-vma 0x80000000 $< $@
	$(STRIP) $@


OEXT		:= .elf
GEN_FILES 	:= $(TARGET).elf32
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
