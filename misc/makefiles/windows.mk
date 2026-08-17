SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/windows
TARGET 		:= ClassiCube

CFLAGS	:= -DUNICODE -fno-ident
LDFLAGS	:= -mwindows
LIBS 	:= -lwinmm
include misc/makefiles/common_config.mk

default: $(TARGET).exe


#-----------------------------
# windres auto detection
#-----------------------------
# tricky because windres is arch and toolchain dependent, but still try to auto guess
# TODO maybe just substitute in path.. ? check windows behaviour
ifneq ($(strip $(WINDRES)),)
    # already set
else ifeq ($(CC),x86_64-w64-mingw32-gcc)
    WINDRES := x86_64-w64-mingw32-windres
else ifeq ($(CC),i686-w64-mingw32-gcc)
    WINDRES := i686-w64-mingw32-windres
else ifeq ($(CC),armv7-w64-mingw32-gcc)
    WINDRES := armv7-w64-mingw32-windres
else ifeq ($(CC),aarch64-w64-mingw32)
    WINDRES := aarch64-w64-mingw32-windres
else
    $(warning WINDRES not set. Generated $(TARGET).exe will not have an icon.)
endif


#-----------------------------
# Icon generation
#-----------------------------
ifneq ($(strip $(WINDRES)),)
ICON_OBJ 	:= $(BUILD_DIR)/ccicon.o
OBJECTS		+= $(ICON_OBJ)

$(ICON_OBJ): misc/windows/CCicon.rc
	$(WINDRES) $< -o $@
endif


#-----------------------------
# Executable generation
#-----------------------------
CC      := gcc
OEXT    := .exe
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
