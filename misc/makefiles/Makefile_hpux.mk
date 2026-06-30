#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/hpux

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -std=c99 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -D_BSD_SOURCE -fvisibility=hidden
# Flags passed to the linker
LDFLAGS = -g -rdynamic
# Libraries to link against
LIBS    = -lm -lX11 -lXi -lXext -L/opt/graphics/OpenGL/lib/hpux32 -lGL -lpthread

CC      = gcc


include misc/makefiles/Makefile_common.mk
