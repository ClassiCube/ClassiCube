#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/freebsd

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -I /usr/local/include
# Flags passed to the linker
LDFLAGS = -L /usr/local/lib -rdynamic
# Libraries to link against
LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread


include misc/makefiles/Makefile_common.mk
