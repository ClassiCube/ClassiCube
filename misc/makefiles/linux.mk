# Detect MCST LCC, where -O3 is about equivalent to -O1
ifeq ($(shell $(CC) -dM -E -xc - < /dev/null | grep -o __MCST__),__MCST__)
	OPT_LEVEL=3
endif


#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/linux

CFLAGS  := -fvisibility=hidden -fno-ident
LDFLAGS	:= -rdynamic
# -lm may be needed for __builtin_sqrtf (in cases where it isn't replaced by a CPU instruction intrinsic)
LIBS 	:= -lX11 -lXi -lpthread -lGL -ldl -lm
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
