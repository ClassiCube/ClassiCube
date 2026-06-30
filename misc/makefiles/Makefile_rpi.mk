#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/rpi

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -fvisibility=hidden -DCC_BUILD_RPI
# Flags passed to the linker
LDFLAGS	= -g -rdynamic
# Libraries to link against
LIBS 	= -lpthread -lX11 -lXi -lEGL -lGLESv2 -ldl

include misc/makefiles/Makefile_common.mk
