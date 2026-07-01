#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/serenity

LDFLAGS	= -g -rdynamic
LIBS 	= -lgl -lSDL2
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
