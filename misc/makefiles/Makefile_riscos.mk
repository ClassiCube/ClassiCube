#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/riscos

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	=
# Flags passed to the linker
LDFLAGS = -g
# Libraries to link against
LIBS    =


include misc/makefiles/Makefile_common.mk
