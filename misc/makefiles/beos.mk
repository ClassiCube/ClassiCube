SOURCE_DIRS := src
BUILD_DIR	:= build/beos
TARGET 		:= ClassiCube

LIBS 	:= -lGL -lnetwork -lbe -lgame -ltracker
include misc/makefiles/common_config.mk
CFLAGS	:= -pipe -fno-math-errno


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
LINK    := $(CXX)
TRACK_DEPENDENCIES = 0
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
