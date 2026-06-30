#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/solaris

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -fvisibility=hidden
# Flags passed to the linker
LDFLAGS	= -g -rdynamic
# Libraries to link against
LIBS 	= -lsocket -lX11 -lXi -lGL

include misc/makefiles/Makefile_common.mk
