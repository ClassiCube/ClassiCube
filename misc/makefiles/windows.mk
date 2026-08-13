SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/windows

CFLAGS	:= -DUNICODE -fno-ident
LDFLAGS	:= -mwindows
LIBS 	:= -lwinmm
include misc/makefiles/common_config.mk


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
.DEFAULT_GOAL := $(TARGET)$(OEXT)


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
