SOURCE_DIRS = src
BUILD_DIR	= build/beos

TARGET  = ClassiCube
CFLAGS	= -pipe -fno-math-errno
LDFLAGS	= -g
LIBS 	= -lGL -lnetwork -lbe -lgame -ltracker

LINK    = $(CXX)
OBJECTS = $(BUILD_DIR)/src/Platform_BeOS.o $(BUILD_DIR)/src/Window_BeOS.o

TRACK_DEPENDENCIES = 0

include misc/makefiles/Makefile_common.mk
