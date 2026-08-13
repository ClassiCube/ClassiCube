SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/flybsd
TARGET 		:= ClassiCube

CFLAGS	:= -I /usr/local/include -fvisibility=hidden -fno-ident
LDFLAGS := -L /usr/local/lib -rdynamic
LIBS    := -lexecinfo -lGL -lX11 -lXi -lpthread
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
