#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/netbsd

CFLAGS	= -I /usr/X11R7/include -I /usr/pkg/include
LDFLAGS = -L /usr/X11R7/lib -L /usr/pkg/lib -rdynamic -Wl,-R/usr/X11R7/lib
LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
include misc/makefiles/common_config.mk


include misc/makefiles/common_build.mk
