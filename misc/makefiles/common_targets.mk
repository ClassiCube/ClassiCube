.PHONY: clean run

#----------------------------------------------------------------
# Determine appropriate shell commands for filesystem operations
#----------------------------------------------------------------
# NOTE: msys treated the same as non-windows systems

ifneq ($(OS),Windows_NT)
    DEL_FILES = rm -f $(1)
else ifneq ($(strip $(MSYS)),)
    DEL_FILES = rm -f $(1)
else
    DEL_FILES = del $(subst /,\,$(1))
endif

ifneq ($(OS),Windows_NT)
    MAKE_DIR = mkdir -p $(1)
else ifneq ($(strip $(MSYS)),)
    MAKE_DIR = mkdir -p $(1)
else
    MAKE_DIR = mkdir $(subst /,\,$(1))
endif


#------------------------------------------------
# Misc targets section
#------------------------------------------------
# Cleans up all built files
clean:
	$(call DEL_FILES,$(GEN_FILES))

# Runs the main executable
run: $(TARGET)$(OEXT)
	$(RUN_PROGRAM) ./$(TARGET)$(OEXT)


# helper macros for packaging files into zip/tar
define DIST_PKG_PATH 
$(BUILD_ROOT)/dist/$(DIST_NAME)/$(1)
endef

define DIST_PKG_ADD 
cp $(1) $(call DIST_PKG_PATH,$(2))
endef

define DIST_PKG_INIT_DEFAULT
$(call MAKE_DIR, $(call DIST_PKG_PATH,audio))
$(call MAKE_DIR, $(call DIST_PKG_PATH,texpacks))
$(call DIST_PKG_ADD,$(1),$(DIST_NAME))
$(call DIST_PKG_ADD,misc/cc_audio.zip,audio/classicube.zip)
$(call DIST_PKG_ADD,misc/cc_textures.zip,texpacks/classicube.zip)
endef

define DIST_PKG_BUILD_TAR
cd $(BUILD_ROOT)/dist && tar -zcvf $(1).tar.gz $(DIST_NAME)
mv $(BUILD_ROOT)/dist/$(1).tar.gz $(1).tar.gz
@echo "----------------------------------------------------"
@echo "Successfully produced bundle: $(1).tar.gz"
@echo "----------------------------------------------------"
endef

define DIST_PKG_BUILD_ZIP
cd $(BUILD_ROOT)/dist && zip -r $(1).zip $(DIST_NAME)
mv $(BUILD_ROOT)/dist/$(1).zip $(1).zip
@echo "----------------------------------------------------"
@echo "Successfully produced bundle: $(1).zip"
@echo "----------------------------------------------------"
endef

