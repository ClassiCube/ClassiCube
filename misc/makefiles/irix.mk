#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/irix

CFLAGS  := -fvisibility=hidden -fno-ident
LDFLAGS	:= -rdynamic
LIBS 	:= -lGL -lX11 -lXi -lpthread -ldl
include misc/makefiles/common_config.mk


CC := gcc
include misc/makefiles/common_build.mk
