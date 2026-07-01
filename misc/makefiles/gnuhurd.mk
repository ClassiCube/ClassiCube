#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/gnuhurd

LDFLAGS = -g -rdynamic
LIBS    = -lX11 -lXi -lpthread -ldl -lm
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
