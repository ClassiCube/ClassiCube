SOURCE_DIRS := src src/beos third_party/bearssl
BUILD_DIR	:= build/haiku
TARGET 		:= ClassiCube
DIST_NAME	:= ClassiCube

CFLAGS  := -fvisibility=hidden -fno-ident
LIBS 	:= -lGL -lnetwork -lbe -lgame -ltracker
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
LINK    := $(CXX)
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk

dist: $(TARGET)
	$(call DIST_PKG_INIT_DEFAULT,$(TARGET))
	$(call DIST_PKG_BUILD_TAR,$(TARGET))
