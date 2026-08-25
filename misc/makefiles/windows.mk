SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/windows
TARGET 		:= ClassiCube

CFLAGS	:= -DUNICODE -fno-ident
LDFLAGS	:= -mwindows
LIBS 	:= -lwinmm
include misc/makefiles/common_config.mk

default: $(TARGET).exe

# tricky because windres is arch and toolchain dependent, but still try to auto guess
WINDRES ?= $(subst gcc,windres,$(subst clang,windres,$(CC)))


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
