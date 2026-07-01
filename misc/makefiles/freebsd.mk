#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/freebsd

CFLAGS	:= -I /usr/local/include -fvisibility=hidden -fno-ident
LDFLAGS := -L /usr/local/lib -rdynamic
LIBS    := -lexecinfo -lGL -lX11 -lXi -lpthread
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
