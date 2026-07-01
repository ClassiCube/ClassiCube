#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/haiku

CFLAGS  := -fvisibility=hidden -fno-ident
LIBS 	:= -lGL -lnetwork -lbe -lgame -ltracker
include misc/makefiles/common_config.mk


LINK    := $(CXX)
OBJECTS := $(BUILD_DIR)/src/Platform_BeOS.o $(BUILD_DIR)/src/Window_BeOS.o
include misc/makefiles/common_build.mk
