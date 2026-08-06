#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src src/wince third_party/bearssl
BUILD_DIR	:= build/wince
WINCE_ARCH	?= armv4

CFLAGS	:= -DUNICODE -D_WIN32_WCE -std=gnu99 -fno-ident
LDFLAGS	:=
LIBS 	:= -lcoredll -lws2
include misc/makefiles/common_config.mk

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


OEXT    := .exe
include misc/makefiles/common_build.mk
