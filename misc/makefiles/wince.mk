#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src src/wince third_party/bearssl
BUILD_DIR	= build/wince

CFLAGS	= -march=armv5te -DUNICODE -D_WIN32_WCE -std=gnu99
LDFLAGS	= -g
LIBS 	= -lcoredll -lws2
include misc/makefiles/common_config.mk


CC      =  arm-mingw32ce-gcc
OEXT    =  .exe
include misc/makefiles/common_build.mk
