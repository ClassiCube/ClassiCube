#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src src/webclient
BUILD_DIR	= build/web

CFLAGS	= -g
LDFLAGS	= -g -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ABORTING_MALLOC=0 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=256Kb --js-library src/webclient/interop_web.js
LIBS 	=
include misc/makefiles/common_config.mk


CC      = emcc
OEXT    = .html
include misc/makefiles/common_build.mk
