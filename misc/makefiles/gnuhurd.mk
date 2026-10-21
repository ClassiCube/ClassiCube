SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/gnuhurd
TARGET 		:= ClassiCube
DIST_NAME	:= ClassiCube

CFLAGS  := -fvisibility=hidden -fno-ident
LDFLAGS := -rdynamic
LIBS    := -lX11 -lXi -lpthread -ldl -lm
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk

dist: $(TARGET)
	$(call DIST_PKG_INIT_DEFAULT,$(TARGET))
	$(call DIST_PKG_BUILD_TAR,$(TARGET))
