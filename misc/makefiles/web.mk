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


#---------------------------------------------------------------------------------
# deployable folder generation
#---------------------------------------------------------------------------------
DIST_DIR := $(BUILD_DIR)/dist

# Packages the webclient into a folder that can be uploaded to a website
# The wheel event fix from doc/compile-fixes.md is applied to the game js
.PHONY: dist
dist: $(TARGET)$(OEXT)
	rm -rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)/static
	cp $(TARGET).html $(DIST_DIR)/index.html
	cp $(TARGET).wasm $(DIST_DIR)/
	sed 's#eventHandler.useCapture);#{ useCapture: eventHandler.useCapture, passive: false });#g' \
	    $(TARGET).js > $(DIST_DIR)/$(TARGET).js
	@if [ -f static/default.zip ]; then cp static/default.zip $(DIST_DIR)/static/; \
	else echo "NOTE: Download https://classicube.net/static/default.zip into $(DIST_DIR)/static/"; fi
	@echo "----------------------------------------------------"
	@echo "Deployable webclient folder: $(DIST_DIR)"
	@echo "----------------------------------------------------"
