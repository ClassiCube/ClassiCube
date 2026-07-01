#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/windows

CFLAGS	= -DUNICODE
LDFLAGS	= -g -mwindows
LIBS 	= -lwinmm
include misc/makefiles/common_config.mk


CC      = gcc
OEXT    = .exe
include misc/makefiles/common_build.mk
