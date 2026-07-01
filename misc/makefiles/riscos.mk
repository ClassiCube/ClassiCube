#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/riscos

CFLAGS  := -fvisibility=hidden
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
