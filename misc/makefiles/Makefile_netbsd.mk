#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/netbsd

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -I /usr/X11R7/include -I /usr/pkg/include
# Flags passed to the linker
LDFLAGS = -L /usr/X11R7/lib -L /usr/pkg/lib -rdynamic -Wl,-R/usr/X11R7/lib
# Libraries to link against
LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread


include misc/makefiles/Makefile_common.mk
