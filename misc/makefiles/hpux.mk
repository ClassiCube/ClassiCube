#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/hpux

CFLAGS	:= -std=c99 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -D_BSD_SOURCE -fvisibility=hidden
LDFLAGS := -rdynamic
LIBS    := -lm -lX11 -lXi -lXext -L/opt/graphics/OpenGL/lib/hpux32 -lGL -lpthread
include misc/makefiles/common_config.mk


CC := gcc
include misc/makefiles/common_build.mk
