SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/riscos
TARGET 		:= ClassiCube

CFLAGS  := -fvisibility=hidden
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
