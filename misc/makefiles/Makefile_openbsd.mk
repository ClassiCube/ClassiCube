#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/openbsd

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -I /usr/X11R6/include -I /usr/local/include
# Flags passed to the linker
LDFLAGS = -L /usr/X11R6/lib -L /usr/local/lib -rdynamic
# Libraries to link against
LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread


include misc/makefiles/Makefile_common.mk
