SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/serenity
TARGET 		:= ClassiCube

CFLAGS  := -fvisibility=hidden -fno-ident
LDFLAGS	:= -rdynamic
LIBS 	:= -lgl -lSDL2
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
