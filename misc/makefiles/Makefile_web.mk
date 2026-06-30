#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src src/webclient
BUILD_DIR	= build/web

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= -g
# Flags passed to the linker
LDFLAGS	= -g -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ABORTING_MALLOC=0 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=256Kb --js-library src/webclient/interop_web.js
# Libraries to link against
LIBS 	=

CC      = emcc
OEXT    = .html

include misc/makefiles/Makefile_common.mk
