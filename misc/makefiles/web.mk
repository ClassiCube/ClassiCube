SOURCE_DIRS := src src/webclient
BUILD_DIR	:= build/web
TARGET 		:= ClassiCube
RUN_PROGRAM	?= emrun

CFLAGS	:= -g
LDFLAGS	:= -g -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ABORTING_MALLOC=0 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=256Kb --js-library src/webclient/interop_web.js --shell-file src/webclient/shell.html
LIBS 	:=
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
CC      := emcc
OEXT    := .html
include misc/makefiles/common_build.mk

# relink when the JS library or page shell changes too
$(TARGET)$(OEXT): src/webclient/interop_web.js src/webclient/shell.html


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
