SOURCE_DIRS := src src/wince third_party/bearssl
BUILD_DIR	:= build/wince
TARGET 		:= ClassiCube
WINCE_ARCH	?= armv4

CFLAGS	:= -DUNICODE -D_WIN32_WCE -std=gnu99 -fno-ident
LDFLAGS	:=
LIBS 	:= -lcoredll -lws2
include misc/makefiles/common_config.mk

default: $(TARGET).exe


#-----------------------------
# Compiler+Tools selection
#-----------------------------
ifeq ($(WINCE_ARCH),armv4)
    CC      := arm-mingw32ce-gcc
    CFLAGS	:= $(CFLAGS) -march=armv4t
else ifeq ($(WINCE_ARCH),armv5)
    CC      := arm-mingw32ce-gcc
    CFLAGS	:= $(CFLAGS) -march=armv5te
else ifeq  ($(WINCE_ARCH),i386)
    CC      := i386-mingw32ce-gcc
else
	$(error "Unknown arch to compile WinCE build for")
endif
WINDRES := $(subst gcc,windres,$(CC))


#-----------------------------
# Icon generation
#-----------------------------
ICON_OBJ := $(BUILD_DIR)/ccicon.o
OBJECTS	 := $(ICON_OBJ)

$(ICON_OBJ): misc/windows/CCicon.rc
	$(WINDRES) $< -o $@


#-----------------------------
# Executable generation
#-----------------------------
OEXT    := .exe
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
