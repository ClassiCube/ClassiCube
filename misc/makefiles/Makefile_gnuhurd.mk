#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/gnuhurd

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -fvisibility=hidden
# Flags passed to the linker
LDFLAGS = -g -rdynamic
# Libraries to link against
LIBS    = -lX11 -lXi -lpthread -ldl -lm


include misc/makefiles/Makefile_common.mk
