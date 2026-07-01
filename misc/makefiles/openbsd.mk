#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/openbsd

CFLAGS	:= -I /usr/X11R6/include -I /usr/local/include -fvisibility=hidden
LDFLAGS := -L /usr/X11R6/lib -L /usr/local/lib -rdynamic
LIBS    := -lexecinfo -lGL -lX11 -lXi -lpthread
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
