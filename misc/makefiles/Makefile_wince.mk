#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/wince

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -march=armv5te -DUNICODE -D_WIN32_WCE -std=gnu99
# Flags passed to the linker
LDFLAGS	= -g
# Libraries to link against
LIBS 	= -lcoredll -lws2

CC      =  arm-mingw32ce-gcc
OEXT    =  .exe


include misc/makefiles/Makefile_common.mk
