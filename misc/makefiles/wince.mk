#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src src/wince third_party/bearssl
BUILD_DIR	:= build/wince
WINCE_ARCH	?= arm

CFLAGS	:= -DUNICODE -D_WIN32_WCE -std=gnu99 -fno-ident
LDFLAGS	:=
LIBS 	:= -lcoredll -lws2
include misc/makefiles/common_config.mk

ifeq ($(WINCE_ARCH),arm)
	CC      := arm-mingw32ce-gcc
	WINCE_CPU ?= armv4t
	CFLAGS	:= $(CFLAGS) -march=$(WINCE_CPU)
else ifeq  ($(WINCE_ARCH),i386)
	CC      := i386-mingw32ce-gcc
else
	$(error "Unknown arch to compile WinCE build for")
endif


OEXT    := .exe
include misc/makefiles/common_build.mk
