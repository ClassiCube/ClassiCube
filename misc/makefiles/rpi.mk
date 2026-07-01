#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/rpi

CFLAGS	:= -fvisibility=hidden -fno-ident -DCC_BUILD_RPI
LDFLAGS	:= -rdynamic
LIBS 	:= -lpthread -lX11 -lXi -lEGL -lGLESv2 -ldl
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
