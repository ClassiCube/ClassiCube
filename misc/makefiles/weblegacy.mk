SOURCE_DIRS := src src/webclient
BUILD_DIR	:= build/weblegacy
TARGET 		:= ClassiCube

CFLAGS	:= -Os -g2
LDFLAGS	:= -g2 -s WASM=0 -s NO_EXIT_RUNTIME=1 -s LEGACY_VM_SUPPORT=1 -s ABORTING_MALLOC=0 -s ALLOW_MEMORY_GROWTH=1 -s ENVIRONMENT=web -s SINGLE_FILE -s TOTAL_STACK=256Kb --closure 0
LIBS 	:= --js-library src/webclient/interop_web.js

default: $(TARGET).js
	sed -i 's#eventHandler.useCapture);#{ useCapture: eventHandler.useCapture, passive: false });#g' ClassiCube.js


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
CC      := emcc
OEXT    := .js
LINK	:= $(CC)
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
