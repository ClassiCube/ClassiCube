#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/haiku

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	=
# Flags passed to the linker
LDFLAGS	= -g
# Libraries to link against
LIBS 	= -lGL -lnetwork -lbe -lgame -ltracker

LINK    = $(CXX)
OBJECTS = $(BUILD_DIR)/src/Platform_BeOS.o $(BUILD_DIR)/src/Window_BeOS.o


include misc/makefiles/Makefile_common.mk
