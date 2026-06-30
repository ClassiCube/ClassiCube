#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/windows

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -DUNICODE
# Flags passed to the linker
LDFLAGS	= -g -mwindows
# Libraries to link against
LIBS 	= -lwinmm

CC      = gcc
OEXT    = .exe


include misc/makefiles/Makefile_common.mk
