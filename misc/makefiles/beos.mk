SOURCE_DIRS := src
BUILD_DIR	:= build/beos

LIBS 	:= -lGL -lnetwork -lbe -lgame -ltracker
include misc/makefiles/common_config.mk
CFLAGS	:= -pipe -fno-math-errno


LINK    := $(CXX)
OBJECTS := $(BUILD_DIR)/src/Platform_BeOS.o $(BUILD_DIR)/src/Window_BeOS.o
TRACK_DEPENDENCIES = 0
include misc/makefiles/common_build.mk
