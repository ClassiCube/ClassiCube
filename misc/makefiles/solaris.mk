#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/solaris

CFLAGS  := -fvisibility=hidden -fno-ident
LDFLAGS	:= -rdynamic
LIBS 	:= -lsocket -lX11 -lXi -lGL
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
